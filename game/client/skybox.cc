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

Vec3f skybox::atmosphere = Vec3f(30.0f, 10.0f, 10.0f);
Vec2f skybox::sun_angles = Vec2f(0.0f, 0.0f);
Vec2f skybox::sun_disk = Vec2f(0.025f, 0.035f);
Vec2f skybox::horizon = Vec2f(0.100f, 0.250f);

Vec3f skybox::sun_direction = Vec3f(0.0f, 0.0f, 0.0f);
Vec3f skybox::fog_color = Vec3f(0.00f, 0.00f, 0.00f);

static VariedProgram program = {};
static std::size_t u_vproj_matrix = {};
static std::size_t u_world_position = {};
static std::size_t u_atmosphere = {};
static std::size_t u_sun_direction = {};
static std::size_t u_sun_disk = {};
static std::size_t u_horizon = {};

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
    u_atmosphere = VariedProgram::add_uniform(program, "u_Atmosphere");
    u_sun_direction = VariedProgram::add_uniform(program, "u_SunDirection");
    u_sun_disk = VariedProgram::add_uniform(program, "u_SunDisk");
    u_horizon = VariedProgram::add_uniform(program, "u_Horizon");

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

    skybox::fog_color[0] = cxpr::clamp(1.0f - std::pow(2.0f, (0.125f + skybox::sun_direction[1]) * (-skybox::atmosphere[0])), 0.0f, 1.0f);
    skybox::fog_color[1] = cxpr::clamp(1.0f - std::pow(2.0f, (0.125f + skybox::sun_direction[1]) * (-skybox::atmosphere[1])), 0.0f, 1.0f);
    skybox::fog_color[2] = cxpr::clamp(1.0f - std::pow(2.0f, (0.125f + skybox::sun_direction[1]) * (-skybox::atmosphere[2])), 0.0f, 1.0f);
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
    glUniform3fv(program.uniforms[u_atmosphere].location, 1, skybox::atmosphere.data());
    glUniform3fv(program.uniforms[u_sun_direction].location, 1, skybox::sun_direction.data());
    glUniform2fv(program.uniforms[u_sun_disk].location, 1, skybox::sun_disk.data());
    glUniform2fv(program.uniforms[u_horizon].location, 1, skybox::horizon.data());

    glBindVertexArray(cube_vaobj);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}

void skybox::layout_debug(void)
{
    ImGui::SliderFloat("azimuth", &skybox::sun_angles[0], 0.0f, 360.0f, "%.0f");
    ImGui::SliderFloat("elevation", &skybox::sun_angles[1], -180.0f, 180.0f, "%.0f");
    ImGui::SliderFloat("atmosphere[0]", &skybox::atmosphere[0], 0.0, 100.0f, "%.0f");
    ImGui::SliderFloat("atmosphere[1]", &skybox::atmosphere[1], 0.0, 100.0f, "%.0f");
    ImGui::SliderFloat("atmosphere[2]", &skybox::atmosphere[2], 0.0, 100.0f, "%.0f");
    ImGui::SliderFloat("sun_disk[0]", &skybox::sun_disk[0], 0.000f, 0.100f, "%.03f");
    ImGui::SliderFloat("sun_disk[1]", &skybox::sun_disk[1], 0.000f, 0.100f, "%.03f");
    ImGui::SliderFloat("horizon[0]", &skybox::horizon[0], 0.000f, 0.250f, "%.03f");
    ImGui::SliderFloat("horizon[1]", &skybox::horizon[1], 0.000f, 0.250f, "%.03f");
}
