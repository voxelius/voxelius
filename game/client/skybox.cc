// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/client/debug_toggles.hh>
#include <game/client/globals.hh>
#include <game/client/settings.hh>
#include <game/client/skybox.hh>
#include <game/client/varied_program.hh>
#include <game/client/view.hh>
#include <mathlib/vec4f.hh>
#include <spdlog/spdlog.h>

Vec2f skybox::sun_angles = Vec2f(45.0f, 0.0f);
Vec3f skybox::sun_direction = Vec3f(0.0f, 0.0f, 0.0f);

float skybox::anisotropic_intensity = 0.0f;
float skybox::atmospheric_density = 0.7f;
float skybox::multiscatter_phase = 0.1f;
float skybox::zenith_offset = 0.0f;

Vec3f skybox::fog_color = Vec3f(0.00f, 0.00f, 0.00f);
Vec3f skybox::sky_color = Vec3f(0.00f, 0.00f, 0.00f);

static VariedProgram program = {};
static std::size_t u_vproj_matrix = {};
static std::size_t u_world_position = {};
static std::size_t u_base_sky_color = {};
static std::size_t u_sun_direction = {};
static std::size_t u_scatter_params = {};

static GLuint cube_vaobj = {};
static GLuint cube_vbo = {};

static Vec3f get_sky_absorption(const Vec3f &x, float y)
{
    Vec3f result = {};
    result[0] = 2.0f * std::exp2(x[0] * (-y));
    result[1] = 2.0f * std::exp2(x[1] * (-y));
    result[2] = 2.0f * std::exp2(x[2] * (-y));
    return std::move(result);
}

static Vec3f jodie_reinhard_tonemap(const Vec3f &color)
{
    const float y = Vec3f::dot(color, Vec3f(0.2126f, 0.7152f, 0.0722f));

    Vec3f tc = {};
    tc[0] = color[0] / (color[0] + 1.0f);
    tc[1] = color[1] / (color[1] + 1.0f);
    tc[2] = color[2] / (color[2] + 1.0f);

    Vec3f result = {};
    result[0] = cxpr::lerp(color[0] / (y + 1.0f), tc[0], tc[0]);
    result[1] = cxpr::lerp(color[1] / (y + 1.0f), tc[1], tc[1]);
    result[2] = cxpr::lerp(color[2] / (y + 1.0f), tc[2], tc[2]);

    result[0] = std::pow(result[0], 2.2f);
    result[1] = std::pow(result[1], 2.2f);
    result[2] = std::pow(result[2], 2.2f);

    return std::move(result);
}

void skybox::init(void)
{
    skybox::sky_color[0] = 0.39f;
    skybox::sky_color[1] = 0.57f;
    skybox::sky_color[2] = 1.00f;

    if(!VariedProgram::setup(program, "shaders/skybox.vert", "shaders/skybox.frag")) {
        spdlog::critical("skybox: program: setup failed");
        std::terminate();
    }

    u_vproj_matrix = VariedProgram::add_uniform(program, "u_ViewProjMatrix");
    u_world_position = VariedProgram::add_uniform(program, "u_WorldPosition");
    u_base_sky_color = VariedProgram::add_uniform(program, "u_BaseSkyColor");
    u_sun_direction = VariedProgram::add_uniform(program, "u_SunDirection");
    u_scatter_params = VariedProgram::add_uniform(program, "u_ScatterParams");

    // https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip
    const Vec3f vertices[14] = {
        Vec3f(-1.0f, +1.0f, +1.0f), // Front-top-left
        Vec3f(+1.0f, +1.0f, +1.0f), // Front-top-right
        Vec3f(-1.0f, -1.0f, +1.0f), // Front-bottom-left
        Vec3f(+1.0f, -1.0f, +1.0f), // Front-bottom-right
        Vec3f(+1.0f, -1.0f, -1.0f), // Back-bottom-right
        Vec3f(+1.0f, +1.0f, +1.0f), // Front-top-right
        Vec3f(+1.0f, +1.0f, -1.0f), // Back-top-right
        Vec3f(-1.0f, +1.0f, +1.0f), // Front-top-left
        Vec3f(-1.0f, +1.0f, -1.0f), // Back-top-left
        Vec3f(-1.0f, -1.0f, +1.0f), // Front-bottom-left
        Vec3f(-1.0f, -1.0f, -1.0f), // Back-bottom-left
        Vec3f(+1.0f, -1.0f, -1.0f), // Back-bottom-right
        Vec3f(-1.0f, +1.0f, -1.0f), // Back-top-left
        Vec3f(+1.0f, +1.0f, -1.0f), // Back-top-right
    };

    glGenVertexArrays(1, &cube_vaobj);
    glBindVertexArray(cube_vaobj);

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), nullptr);
}

void skybox::deinit(void)
{
    glDeleteBuffers(1, &cube_vbo);
    glDeleteVertexArrays(1, &cube_vaobj);
    VariedProgram::destroy(program);
}

void skybox::update(void)
{
    sun_angles[1] += 5.0f * globals::frametime;

    const float az_sin = std::sin(cxpr::radians(skybox::sun_angles[0]));
    const float az_cos = std::cos(cxpr::radians(skybox::sun_angles[0]));
    const float el_sin = std::sin(cxpr::radians(skybox::sun_angles[1]));
    const float el_cos = std::cos(cxpr::radians(skybox::sun_angles[1]));

    skybox::sun_direction[0] = az_sin * el_cos;
    skybox::sun_direction[1] = az_cos * el_cos;
    skybox::sun_direction[2] = el_sin;

    Vec3f base_sky = skybox::sky_color * (1.0f + skybox::anisotropic_intensity);
    const float zenith = skybox::atmospheric_density / std::pow(cxpr::max(skybox::sun_direction[1] - skybox::zenith_offset, 0.0035f), 0.75f);
    const Vec3f absorption = get_sky_absorption(base_sky, zenith);
    skybox::fog_color[0] = absorption[0] * base_sky[0] * zenith;
    skybox::fog_color[1] = absorption[1] * base_sky[1] * zenith;
    skybox::fog_color[2] = absorption[2] * base_sky[2] * zenith;

    // FIXME: make it faster
    skybox::fog_color = jodie_reinhard_tonemap(skybox::fog_color);
}

void skybox::render(void)
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.0f);

    if(debug_toggles::render_wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if(!VariedProgram::update(program)) {
        spdlog::critical("skybox: program: update failed");
        VariedProgram::destroy(program);
        std::terminate();
    }

    Vec4f params = {};
    params[0] = skybox::anisotropic_intensity;
    params[1] = skybox::atmospheric_density;
    params[2] = skybox::multiscatter_phase;
    params[3] = skybox::zenith_offset;

    glUseProgram(program.handle);
    glUniformMatrix4fv(program.uniforms[u_vproj_matrix].location, 1, false, view::matrix.data()->data());
    glUniform3fv(program.uniforms[u_world_position].location, 1, view::position.local.data());
    glUniform3fv(program.uniforms[u_base_sky_color].location, 1, skybox::sky_color.data());
    glUniform3fv(program.uniforms[u_sun_direction].location, 1, skybox::sun_direction.data());
    glUniform4fv(program.uniforms[u_scatter_params].location, 1, params.data());

    glBindVertexArray(cube_vaobj);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}
