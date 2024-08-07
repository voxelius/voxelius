// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/outline_renderer.hh>
#include <client/view.hh>
#include <spdlog/spdlog.h>

static GLuint program = {};
static GLuint vaobj = {};
static GLuint cube_vbo = {};
static GLuint line_vbo = {};

static GLint u_vproj_matrix = {};
static GLint u_world_position = {};
static GLint u_scale = {};
static GLint u_color = {};

void outline_renderer::init(void)
{
    GLuint vert = util::compile_shader("shaders/outline.vert", GL_VERTEX_SHADER);
    GLuint frag = util::compile_shader("shaders/outline.frag", GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        spdlog::critical("outline_renderer: shader compile failed");
        std::terminate();
    }

    // A single program is used. Internally it keeps
    // an array of transformations depending on what
    // instance is being rendered currently. At most
    // the progam can be called for six instances with
    // instance zero being also used for line drawing.
    // It is obscure but it allows me to use less OpenGL
    program = util::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!program) {
        spdlog::critical("outline_renderer: program link failed");
        std::terminate();
    }

    const Vec3f cube_vertices[4] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
    };

    const Vec3f line_vertices[2] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 1.0f),
    };

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);

    u_vproj_matrix = glGetUniformLocation(program, "u_ViewProjMatrix");
    u_world_position = glGetUniformLocation(program, "u_WorldPosition");
    u_scale = glGetUniformLocation(program, "u_Scale");
    u_color = glGetUniformLocation(program, "u_Color");
}

void outline_renderer::deinit(void)
{
    glDeleteBuffers(1, &line_vbo);
    glDeleteBuffers(1, &cube_vbo);
    glDeleteVertexArrays(1, &vaobj);
    glDeleteProgram(program);
}

void outline_renderer::prepare_depth(void)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUseProgram(program);
    glUniformMatrix4fv(u_vproj_matrix, 1, false, view::matrix.data()->data());

    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);
}

void outline_renderer::prepare_nodepth(void)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUseProgram(program);
    glUniformMatrix4fv(u_vproj_matrix, 1, false, view::matrix.data()->data());

    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);
}

void outline_renderer::draw_cube(const WorldCoord &start, const Vec3f &scale, float width, const Vec4f &color)
{
    WorldCoord patch = start;
    patch.chunk -= view::position.chunk;

    glLineWidth(width);

    glUniform3fv(u_world_position, 1, WorldCoord::to_vec3f(patch).data());
    glUniform3fv(u_scale, 1, scale.data());
    glUniform4fv(u_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);
    glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 6);
}

void outline_renderer::draw_line(const WorldCoord &start, const Vec3f &scale, float width, const Vec4f &color)
{
    WorldCoord patch = start;
    patch.chunk -= view::position.chunk;

    glLineWidth(width);

    glUniform3fv(u_world_position, 1, WorldCoord::to_vec3f(patch).data());
    glUniform3fv(u_scale, 1, scale.data());
    glUniform4fv(u_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);
    glDrawArrays(GL_LINES, 0, 2);
}

