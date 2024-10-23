// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024, untodesu
#include <game/client/game.hh>
#include <game/client/linedraw.hh>
#include <game/client/varied_program.hh>
#include <game/client/view.hh>
#include <spdlog/spdlog.h>

// ONLY TOUCH THESE IF THE RESPECTIVE SHADER
// VARIANT MACRO DECLARATIONS LAYOUT CHANGED AS WELL
constexpr static unsigned int WORLD_CURVATURE = 0U;

static VariedProgram program = {};
static std::size_t u_vpmatrix = {};
static std::size_t u_worldpos = {};
static std::size_t u_viewdistance = {};
static std::size_t u_modulate = {};
static std::size_t u_scale = {};
static GLuint vaobj = {};
static GLuint vbo = {};

void linedraw::init(void)
{
    if(!VariedProgram::setup(program, "shaders/linedraw.vert", "shaders/linedraw.frag")) {
        spdlog::critical("linedraw: program setup failed");
        std::terminate();
    }

    u_vpmatrix = VariedProgram::add_uniform(program, "u_ViewProjMatrix");
    u_worldpos = VariedProgram::add_uniform(program, "u_WorldPosition");
    u_viewdistance = VariedProgram::add_uniform(program, "u_ViewDistance");
    u_modulate = VariedProgram::add_uniform(program, "u_Modulate");
    u_scale = VariedProgram::add_uniform(program, "u_Scale");

    const Vec3f vertices[2] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 1.0f),
    };

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);
}

void linedraw::deinit(void)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vaobj);
    VariedProgram::destroy(program);
}

void linedraw::render(const WorldCoord &offset, const Vec3f &line, float thickness, const Vec4f &color)
{
    VariedProgram::set_variant_vert(program, WORLD_CURVATURE, client_game::world_curvature);

    if(!VariedProgram::update(program)) {
        spdlog::critical("linedraw: program update failed");
        std::terminate();
    }

    WorldCoord patch = offset;
    patch.chunk -= view::position.chunk;

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(program.handle);
    glUniformMatrix4fv(program.uniforms[u_vpmatrix].location, 1, false, view::matrix.data()->data());
    glUniform3fv(program.uniforms[u_worldpos].location, 1, WorldCoord::to_vec3f(patch).data());
    glUniform1f(program.uniforms[u_viewdistance].location, view::max_distance * CHUNK_SIZE);
    glUniform4fv(program.uniforms[u_modulate].location, 1, color.data());
    glUniform3fv(program.uniforms[u_scale].location, 1, line.data());

    glLineWidth(thickness);

    glBindVertexArray(vaobj);
    glDrawArrays(GL_LINES, 0, 2);
}
