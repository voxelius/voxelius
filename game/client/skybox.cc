// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/client/debug_toggles.hh>
#include <game/client/globals.hh>
#include <game/client/skybox.hh>
#include <game/client/varied_program.hh>
#include <game/client/view.hh>
#include <imgui.h>
#include <mathlib/vec4base.hh>
#include <random>
#include <spdlog/spdlog.h>

float skybox::anisotropic_intensity = 0.0f;
float skybox::atmospheric_density = 0.50f;
float skybox::multiscatter_phase = 0.35f;
float skybox::zenith_offset = 0.0f;
unsigned int skybox::stars_gamma = 2048U;
unsigned int skybox::stars_zoom = 256U;
unsigned int skybox::stars_seed = 42U;

Vec3f skybox::sky_color = Vec3f(0.39f, 0.57f, 1.00f);
Vec2f skybox::sun_angles = Vec2f(0.00f, 0.00f);

Vec3f skybox::sun_direction = Vec3f(0.0f, 0.0f, 0.0f);
Vec3f skybox::fog_color = Vec3f(0.0f, 0.0f, 0.0f);

static VariedProgram program = {};
static std::size_t u_vproj_matrix = {};
static std::size_t u_world_position = {};
static std::size_t u_scatter = {};
static std::size_t u_stars = {};
static std::size_t u_sky_color = {};
static std::size_t u_sun_direction = {};

static GLuint cube_vaobj = {};
static GLuint cube_vbo = {};

static Vec3f as_absorption(const Vec3f &point, float y)
{
    Vec3f result = {};
    result[0] = 2.0f * std::exp2(point[0] * (-y));
    result[1] = 2.0f * std::exp2(point[1] * (-y));
    result[2] = 2.0f * std::exp2(point[2] * (-y));
    return std::move(result);
}

static float as_zenith_density(float x)
{
    return skybox::atmospheric_density / std::pow(cxpr::max(x - skybox::zenith_offset, 0.0035f), 0.75f);
}

void skybox::init(void)
{
    if(!VariedProgram::setup(program, "shaders/skybox.vert", "shaders/skybox.frag")) {
        spdlog::critical("skybox: program: setup failed");
        std::terminate();
    }

    u_vproj_matrix = VariedProgram::add_uniform(program, "u_ViewProjMatrix");
    u_world_position = VariedProgram::add_uniform(program, "u_WorldPosition");
    u_scatter = VariedProgram::add_uniform(program, "u_Scatter");
    u_stars = VariedProgram::add_uniform(program, "u_Stars");
    u_sky_color = VariedProgram::add_uniform(program, "u_SkyColor");
    u_sun_direction = VariedProgram::add_uniform(program, "u_SunDirection");

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
    const float az_sin = std::sin(cxpr::radians(skybox::sun_angles[0]));
    const float az_cos = std::cos(cxpr::radians(skybox::sun_angles[0]));
    const float el_sin = std::sin(cxpr::radians(skybox::sun_angles[1]));
    const float el_cos = std::cos(cxpr::radians(skybox::sun_angles[1]));

    // https://www.desmos.com/3d/8npmtkuc3k
    skybox::sun_direction[0] = az_sin * el_cos;
    skybox::sun_direction[1] = el_sin;
    skybox::sun_direction[2] = az_cos * el_cos;

    const float sun_point_dist_mult = cxpr::clamp(skybox::sun_direction[1] + skybox::multiscatter_phase - skybox::zenith_offset, 0.0f, 1.0f);
    const float zenith = as_zenith_density(skybox::zenith_offset);

    const Vec3f sky_color_amp = skybox::sky_color + skybox::sky_color * skybox::anisotropic_intensity;
    const Vec3f sun_absorption = as_absorption(sky_color_amp, as_zenith_density(skybox::sun_direction[1] + skybox::multiscatter_phase));
    const Vec3f sky_absorption = as_absorption(sky_color_amp, zenith);
    const Vec3f sky = sky_color_amp * zenith;

    skybox::fog_color[0] = cxpr::lerp(sky[0] * sky_absorption[0], sky[0] / (sky[0] + 0.5f), sun_point_dist_mult);
    skybox::fog_color[1] = cxpr::lerp(sky[1] * sky_absorption[1], sky[1] / (sky[1] + 0.5f), sun_point_dist_mult);
    skybox::fog_color[2] = cxpr::lerp(sky[2] * sky_absorption[2], sky[2] / (sky[2] + 0.5f), sun_point_dist_mult);

    const float sun_absorption_mag = Vec3f::length(sun_absorption);
    skybox::fog_color[0] *= sun_absorption[0] * 0.5f + 0.5f * sun_absorption_mag;
    skybox::fog_color[1] *= sun_absorption[1] * 0.5f + 0.5f * sun_absorption_mag;
    skybox::fog_color[2] *= sun_absorption[2] * 0.5f + 0.5f * sun_absorption_mag;

    skybox::fog_color[0] *= static_cast<float>(M_PI);
    skybox::fog_color[1] *= static_cast<float>(M_PI);
    skybox::fog_color[2] *= static_cast<float>(M_PI);

    const float lum = Vec3f::dot(skybox::fog_color, Vec3f(0.2126f, 0.7152f, 0.0722f));
    const float tx = skybox::fog_color[0] / (skybox::fog_color[0] + 1.0f);
    const float ty = skybox::fog_color[1] / (skybox::fog_color[1] + 1.0f);
    const float tz = skybox::fog_color[2] / (skybox::fog_color[2] + 1.0f);

    skybox::fog_color[0] = cxpr::lerp(skybox::fog_color[0] / (lum + 1.0f), tx, tx);
    skybox::fog_color[1] = cxpr::lerp(skybox::fog_color[1] / (lum + 1.0f), ty, ty);
    skybox::fog_color[2] = cxpr::lerp(skybox::fog_color[2] / (lum + 1.0f), tz, tz);

    skybox::fog_color[0] = std::pow(skybox::fog_color[0], 2.2f);
    skybox::fog_color[1] = std::pow(skybox::fog_color[1], 2.2f);
    skybox::fog_color[2] = std::pow(skybox::fog_color[2], 2.2f);

    //skybox::fog_color[0] = cxpr::clamp(1.0f - std::pow(2.0f, (0.25 + skybox::sun_direction[1]) * (-skybox::absorption[0])), 0.0f, 1.0f);
    //skybox::fog_color[1] = cxpr::clamp(1.0f - std::pow(2.0f, (0.25 + skybox::sun_direction[1]) * (-skybox::absorption[1])), 0.0f, 1.0f);
    //skybox::fog_color[2] = cxpr::clamp(1.0f - std::pow(2.0f, (0.25 + skybox::sun_direction[1]) * (-skybox::absorption[2])), 0.0f, 1.0f);
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

    Vec4base<GLfloat> scatter = {};
    scatter[0] = static_cast<GLfloat>(skybox::anisotropic_intensity);
    scatter[1] = static_cast<GLfloat>(skybox::atmospheric_density);
    scatter[2] = static_cast<GLfloat>(skybox::multiscatter_phase);
    scatter[3] = static_cast<GLfloat>(skybox::zenith_offset);

    Vec3base<GLuint> stars = {};
    stars[0] = static_cast<GLuint>(skybox::stars_gamma);
    stars[1] = static_cast<GLuint>(skybox::stars_zoom);
    stars[2] = static_cast<GLuint>(skybox::stars_seed);

    glUseProgram(program.handle);
    glUniformMatrix4fv(program.uniforms[u_vproj_matrix].location, 1, false, view::matrix.data()->data());
    glUniform3fv(program.uniforms[u_world_position].location, 1, view::position.local.data());
    glUniform4fv(program.uniforms[u_scatter].location, 1, scatter.data());
    glUniform3uiv(program.uniforms[u_stars].location, 1, stars.data());
    glUniform3fv(program.uniforms[u_sky_color].location, 1, skybox::sky_color.data());
    glUniform3fv(program.uniforms[u_sun_direction].location, 1, skybox::sun_direction.data());

    glBindVertexArray(cube_vaobj);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}

void skybox::layout_debug(void)
{
    ImGui::SliderFloat("sun_angles[AZ]", &skybox::sun_angles[0], 0.0f, 360.0f, "%.0f");
    ImGui::SliderFloat("sun_angles[EL]", &skybox::sun_angles[1], -180.0f, 180.0f, "%.0f");

    ImGui::SeparatorText("Atmosphere");
    ImGui::SliderFloat("anisotropic_intensity", &skybox::anisotropic_intensity, 0.00f, 1.00f, "%.02f");
    ImGui::SliderFloat("atmospheric_density", &skybox::atmospheric_density, 0.00f, 1.00f, "%.02f");
    ImGui::SliderFloat("multiscatter_phase", &skybox::multiscatter_phase, 0.00f, 1.00f, "%.02f");
    ImGui::SliderFloat("zenith_offset", &skybox::zenith_offset, -1.00f, 1.00f, "%.02f");
    ImGui::ColorEdit3("sky_color", skybox::sky_color.data());

    ImGui::SeparatorText("Stars");
    int stars_gamma_v = static_cast<int>(skybox::stars_gamma);
    int stars_zoom_v = static_cast<int>(skybox::stars_zoom);    
    int stars_seed_v = static_cast<int>(skybox::stars_seed);
    ImGui::SliderInt("stars_gamma", &stars_gamma_v, 0, 2048);
    ImGui::SliderInt("stars_zoom", &stars_zoom_v, 0, 2048);
    ImGui::InputInt("stars_seed", &stars_seed_v); ImGui::SameLine();
    if(ImGui::Button("R###stars_seed.RBTN")) stars_seed_v = static_cast<int>(std::random_device()() % std::numeric_limits<int>::max());
    skybox::stars_gamma = cxpr::clamp<unsigned int>(stars_gamma_v, 0U, 2048U);
    skybox::stars_zoom = cxpr::clamp<unsigned int>(stars_zoom_v, 1U, 1024U);
    skybox::stars_seed = static_cast<unsigned int>(stars_seed_v);
}
