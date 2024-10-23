// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024, untodesu
#include <game/client/cubedraw.hh>
#include <game/client/game.hh>
#include <game/client/varied_program.hh>
#include <game/client/view.hh>
#include <spdlog/spdlog.h>

// ONLY TOUCH THESE IF THE RESPECTIVE SHADER
// VARIANT MACRO DECLARATIONS LAYOUT CHANGED AS WELL
constexpr static unsigned int WORLD_CURVATURE = 0U;

static VariedProgram mprogram = {};
static std::size_t u_m_vpmatrix = {};
static std::size_t u_m_mdmatrix = {};
static std::size_t u_m_modulate = {};

static VariedProgram wprogram = {};
static std::size_t u_w_vpmatrix = {};
static std::size_t u_w_worldpos = {};
static std::size_t u_w_viewdistance = {};
static std::size_t u_w_modulate = {};
static std::size_t u_w_scale = {};

static GLuint vaobj = {};
static GLuint vbo = {};

void cubedraw::init(void)
{
    if(!VariedProgram::setup(mprogram, "shaders/cubedraw_model.vert", "shaders/cubedraw.frag")) {
        spdlog::critical("cubedraw: model program setup failed");
        std::terminate();
    }

    u_m_vpmatrix = VariedProgram::add_uniform(mprogram, "u_ViewProjMatrix");
    u_m_mdmatrix = VariedProgram::add_uniform(mprogram, "u_ModelMatrix");
    u_m_modulate = VariedProgram::add_uniform(mprogram, "u_Modulate");

    if(!VariedProgram::setup(wprogram, "shaders/cubedraw_wpos.vert", "shaders/cubedraw.frag")) {
        spdlog::critical("cubedraw: wpos program setup failed");
        std::terminate();
    }

    u_w_vpmatrix = VariedProgram::add_uniform(wprogram, "u_ViewProjMatrix");
    u_w_worldpos = VariedProgram::add_uniform(wprogram, "u_WorldPosition");
    u_w_viewdistance = VariedProgram::add_uniform(wprogram, "u_ViewDistance");
    u_w_modulate = VariedProgram::add_uniform(wprogram, "u_Modulate");
    u_w_scale = VariedProgram::add_uniform(wprogram, "u_Scale");

    const Vec3f vertices[4] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
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

void cubedraw::deinit(void)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vaobj);
    VariedProgram::destroy(wprogram);
    VariedProgram::destroy(mprogram);
}

void cubedraw::render(const Mat4x4f &model, const Mat4x4f &view, float thickness, const Vec4f &color)
{
    if(!VariedProgram::update(mprogram)) {
        spdlog::critical("cubedraw: wpos program update failed");
        std::terminate();
    }

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(mprogram.handle);
    glUniformMatrix4fv(mprogram.uniforms[u_m_vpmatrix].location, 1, false, view.data()->data());
    glUniformMatrix4fv(mprogram.uniforms[u_m_mdmatrix].location, 1, false, model.data()->data());
    glUniform4fv(mprogram.uniforms[u_m_modulate].location, 1, color.data());

    glLineWidth(thickness);

    glBindVertexArray(vaobj);
    glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 6);
}

void cubedraw::render(const WorldCoord &offset, const Vec3f &size, float thickness, const Vec4f &color)
{
    VariedProgram::set_variant_vert(wprogram, WORLD_CURVATURE, client_game::world_curvature);

    if(!VariedProgram::update(wprogram)) {
        spdlog::critical("cubedraw: wpos program update failed");
        std::terminate();
    }

    WorldCoord patch = offset;
    patch.chunk -= view::position.chunk;

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(wprogram.handle);
    glUniformMatrix4fv(wprogram.uniforms[u_w_vpmatrix].location, 1, false, view::matrix.data()->data());
    glUniform3fv(wprogram.uniforms[u_w_worldpos].location, 1, WorldCoord::to_vec3f(patch).data());
    glUniform1f(wprogram.uniforms[u_w_viewdistance].location, view::max_distance * CHUNK_SIZE);
    glUniform4fv(wprogram.uniforms[u_w_modulate].location, 1, color.data());
    glUniform3fv(wprogram.uniforms[u_w_scale].location, 1, size.data());

    glLineWidth(thickness);

    glBindVertexArray(vaobj);
    glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 6);
}
