// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024, untodesu
#include <game/client/game.hh>
#include <game/client/outline.hh>
#include <game/client/varied_program.hh>
#include <game/client/view.hh>
#include <spdlog/spdlog.h>

// ONLY TOUCH THESE IF THE RESPECTIVE SHADER
// VARIANT MACRO DECLARATIONS LAYOUT CHANGED AS WELL
constexpr static unsigned int WORLD_CURVATURE = 0U;

static VariedProgram program = {};
static std::size_t u_vpmatrix = {};
static std::size_t u_worldpos = {};
static std::size_t u_viewdist = {};
static std::size_t u_modulate = {};
static std::size_t u_scale = {};

static GLuint vaobj = {};
static GLuint cube_vbo = {};
static GLuint line_vbo = {};

void outline::init(void)
{
    if(!VariedProgram::setup(program, "shaders/outline.vert", "shaders/outline.frag")) {
        spdlog::critical("outline: program setup failed");
        std::terminate();
    }

    u_vpmatrix = VariedProgram::add_uniform(program, "u_ViewProjMatrix");
    u_worldpos = VariedProgram::add_uniform(program, "u_WorldPosition");
    u_viewdist = VariedProgram::add_uniform(program, "u_ViewDistance");
    u_modulate = VariedProgram::add_uniform(program, "u_Modulate");
    u_scale = VariedProgram::add_uniform(program, "u_Scale");

    const Vec3f cube_vertices[24] = {
        Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f),
        Vec3f(0.0f, 1.0f, 0.0f), Vec3f(1.0f, 1.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 0.0f), Vec3f(1.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, 0.0f),

        Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, 1.0f, 1.0f),
        Vec3f(0.0f, 1.0f, 1.0f), Vec3f(1.0f, 1.0f, 1.0f),
        Vec3f(1.0f, 1.0f, 1.0f), Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 1.0f), Vec3f(0.0f, 0.0f, 1.0f),

        Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 1.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 0.0f), Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 1.0f, 0.0f), Vec3f(1.0f, 1.0f, 1.0f),
    };

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    const Vec3f line_vertices[2] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 1.0f),
    };

    glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vaobj);

    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
}

void outline::deinit(void)
{
    glDeleteVertexArrays(1, &vaobj);
    glDeleteBuffers(1, &line_vbo);
    glDeleteBuffers(1, &cube_vbo);
    VariedProgram::destroy(program);
}

void outline::prepare(void)
{
    VariedProgram::set_variant_vert(program, WORLD_CURVATURE, client_game::world_curvature);

    if(!VariedProgram::update(program)) {
        spdlog::critical("outline_renderer: program update failed");
        std::terminate();
    }

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(program.handle);
    glUniformMatrix4fv(program.uniforms[u_vpmatrix].location, 1, false, view::matrix.data()->data());
    glUniform1f(program.uniforms[u_viewdist].location, CHUNK_SIZE * view::max_distance);

    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
}

void outline::cube(const WorldCoord &offset, const Vec3f &size, float thickness, const Vec4f &color)
{
    WorldCoord patch = {};
    patch.chunk = offset.chunk - view::position.chunk;
    patch.local = offset.local;

    glLineWidth(thickness);

    glUniform3fv(program.uniforms[u_worldpos].location, 1, WorldCoord::to_vec3f(patch).data());
    glUniform4fv(program.uniforms[u_modulate].location, 1, color.data());
    glUniform3fv(program.uniforms[u_scale].location, 1, size.data());

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);
    glDrawArrays(GL_LINES, 0, 24);
}

void outline::line(const WorldCoord &offset, const Vec3f &line, float thickness, const Vec4f &color)
{
    WorldCoord patch = {};
    patch.chunk = offset.chunk - view::position.chunk;
    patch.local = offset.local;

    glLineWidth(thickness);

    glUniform3fv(program.uniforms[u_worldpos].location, 1, WorldCoord::to_vec3f(patch).data());
    glUniform4fv(program.uniforms[u_modulate].location, 1, color.data());
    glUniform3fv(program.uniforms[u_scale].location, 1, line.data());

    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);
    glDrawArrays(GL_LINES, 0, 2);
}
