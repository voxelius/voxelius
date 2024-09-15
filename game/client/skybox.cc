// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/client/debug_toggles.hh>
#include <game/client/globals.hh>
#include <game/client/skybox.hh>
#include <game/client/varied_program.hh>
#include <game/client/view.hh>
#include <imgui.h>
#include <mathlib/vec4f.hh>
#include <spdlog/spdlog.h>

Vec3f skybox::absorption = Vec3f(100.0f, 20.0f, 20.0f);
float skybox::horizon_scale = 0.25f;
float skybox::horizon_start = -0.100f;
float skybox::horizon_end = 0.200f;
float skybox::stars_gamma = 2.2f;
unsigned int skybox::stars_scale = 256U;
unsigned int skybox::stars_seed = 42U;
float skybox::twilight_offset = 0.25f;
Vec2f skybox::sun_angles = Vec2f(0.0f, 0.0f);
Vec3f skybox::sun_direction = Vec3f(0.0f, 0.0f, 0.0f);
Vec3f skybox::fog_color = Vec3f(0.0f, 0.0f, 0.0f);

static VariedProgram program = {};
static std::size_t u_vproj_matrix = {};
static std::size_t u_world_position = {};
static std::size_t u_absorption = {};
static std::size_t u_horizon = {};
static std::size_t u_stars = {};
static std::size_t u_sun_direction = {};
static std::size_t u_twilight_offset = {};

static GLuint cube_vaobj = {};
static GLuint cube_vbo = {};

void skybox::init(void)
{
    if(!VariedProgram::setup(program, "shaders/skybox.vert", "shaders/skybox.frag")) {
        spdlog::critical("skybox: program: setup failed");
        std::terminate();
    }

    u_vproj_matrix = VariedProgram::add_uniform(program, "u_ViewProjMatrix");
    u_world_position = VariedProgram::add_uniform(program, "u_WorldPosition");
    u_absorption = VariedProgram::add_uniform(program, "u_Absorption");
    u_horizon = VariedProgram::add_uniform(program, "u_Horizon");
    u_stars = VariedProgram::add_uniform(program, "u_Stars");
    u_sun_direction = VariedProgram::add_uniform(program, "u_SunDirection");
    u_twilight_offset = VariedProgram::add_uniform(program, "u_TwilightOffset");

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

    skybox::fog_color[0] = cxpr::clamp(1.0f - std::pow(2.0f, (skybox::twilight_offset + skybox::sun_direction[1]) * (-skybox::absorption[0])), 0.0f, 1.0f);
    skybox::fog_color[1] = cxpr::clamp(1.0f - std::pow(2.0f, (skybox::twilight_offset + skybox::sun_direction[1]) * (-skybox::absorption[1])), 0.0f, 1.0f);
    skybox::fog_color[2] = cxpr::clamp(1.0f - std::pow(2.0f, (skybox::twilight_offset + skybox::sun_direction[1]) * (-skybox::absorption[2])), 0.0f, 1.0f);
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

    glUseProgram(program.handle);
    glUniformMatrix4fv(program.uniforms[u_vproj_matrix].location, 1, false, view::matrix.data()->data());
    glUniform3fv(program.uniforms[u_world_position].location, 1, view::position.local.data());
    glUniform3fv(program.uniforms[u_absorption].location, 1, skybox::absorption.data());
    glUniform3f(program.uniforms[u_horizon].location, skybox::horizon_scale, skybox::horizon_start, skybox::horizon_end);
    glUniform3f(program.uniforms[u_stars].location, skybox::stars_gamma, skybox::stars_scale, skybox::stars_seed);
    glUniform3fv(program.uniforms[u_sun_direction].location, 1, skybox::sun_direction.data());
    glUniform1f(program.uniforms[u_twilight_offset].location, skybox::twilight_offset);

    glBindVertexArray(cube_vaobj);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}

void skybox::layout_debug(void)
{
    ImGui::SliderFloat("sun_angles[AZ]", &skybox::sun_angles[0], 0.0f, 360.0f, "%.0f");
    ImGui::SliderFloat("sun_angles[EL]", &skybox::sun_angles[1], -180.0f, 180.0f, "%.0f");

    ImGui::SeparatorText("Atmosphere");
    ImGui::SliderFloat("absorption[R]", &skybox::absorption[0], 0.0f, 100.0f, "%.0f");
    ImGui::SliderFloat("absorption[G]", &skybox::absorption[1], 0.0f, 100.0f, "%.0f");
    ImGui::SliderFloat("absorption[B]", &skybox::absorption[2], 0.0f, 100.0f, "%.0f");
    ImGui::SliderFloat("horizon_scale", &skybox::horizon_scale, 0.00f, 1.00f, "%.02f");
    ImGui::SliderFloat("horizon_start", &skybox::horizon_start, -1.000f, 1.000f, "%.03f");
    ImGui::SliderFloat("horizon_end", &skybox::horizon_end, -1.000f, 1.000f, "%.03f");
    ImGui::SliderFloat("twilight_offset", &skybox::twilight_offset, 0.00f, 1.00f, "%.02f");

    ImGui::SeparatorText("Stars");
    int stars_seed_v = static_cast<int>(skybox::stars_seed);
    int stars_scale_v = static_cast<int>(skybox::stars_scale);
    ImGui::SliderFloat("stars_gamma", &skybox::stars_gamma, 0.0f, 10.0f, "%.01f");
    ImGui::SliderInt("stars_scale", &stars_scale_v, 0, 1024);
    ImGui::InputInt("stars_seed", &stars_seed_v);
    skybox::stars_scale = cxpr::clamp<unsigned int>(stars_scale_v, 1U, 1024U);
    skybox::stars_seed = static_cast<unsigned int>(stars_seed_v);
}
