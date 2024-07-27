// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/camera.hh>
#include <client/debug_draw.hh>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

static GLuint draw_program = {};
static GLuint draw_vaobj = {};
static GLuint cube_vbo = {};
static GLuint line_vbo = {};

static GLint u_camera_matrix = {};
static GLint u_world_position = {};
static GLint u_scale = {};
static GLint u_color = {};

void debug_draw::init(void)
{
    GLuint vert = util::compile_shader("shaders/debug_draw.vert", GL_VERTEX_SHADER);
    GLuint frag = util::compile_shader("shaders/debug_draw.frag", GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        spdlog::critical("debug_draw: shader compile failed");
        std::terminate();
    }

    // A single program is used. Internally it keeps
    // an array of transformations depending on what
    // instance is being rendered currently. At most
    // the progam can be called for six instances with
    // instance zero being also used for line drawing.
    // It is obscure but it allows me to use less OpenGL
    draw_program = util::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!draw_program) {
        spdlog::critical("debug_draw: program link failed");
        std::terminate();
    }

    const glm::fvec3 cube_vertices[4] = {
        glm::fvec3(0.0f, 0.0f, 0.0f),
        glm::fvec3(0.0f, 0.0f, 1.0f),
        glm::fvec3(1.0f, 0.0f, 1.0f),
        glm::fvec3(1.0f, 0.0f, 0.0f),
    };

    const glm::fvec3 line_vertices[2] = {
        glm::fvec3(0.0f, 0.0f, 0.0f),
        glm::fvec3(1.0f, 1.0f, 1.0f),
    };

    glGenVertexArrays(1, &draw_vaobj);
    glBindVertexArray(draw_vaobj);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);

    u_camera_matrix = glGetUniformLocation(draw_program, "u_CameraMatrix");
    u_world_position = glGetUniformLocation(draw_program, "u_WorldPosition");
    u_scale = glGetUniformLocation(draw_program, "u_Scale");
    u_color = glGetUniformLocation(draw_program, "u_Color");
}

void debug_draw::deinit(void)
{
    glDeleteBuffers(1, &line_vbo);
    glDeleteBuffers(1, &cube_vbo);
    glDeleteVertexArrays(1, &draw_vaobj);
    glDeleteProgram(draw_program);
}

void debug_draw::begin(bool depth_testing)
{
    if(depth_testing)
        glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUseProgram(draw_program);
    glUniformMatrix4fv(u_camera_matrix, 1, false, glm::value_ptr(camera::matrix()));

    glBindVertexArray(draw_vaobj);
    glEnableVertexAttribArray(0);
}

void debug_draw::cube(const EntityPos &start, const glm::fvec3 &scale, float width, const glm::fvec3 &color)
{
    EntityPos patch = start;
    patch.chunk -= camera::position().chunk;

    glLineWidth(width);
    
    glUniform3fv(u_world_position, 1, glm::value_ptr(coord::to_world(patch)));
    glUniform3fv(u_scale, 1, glm::value_ptr(scale));
    glUniform3fv(u_color, 1, glm::value_ptr(color));

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(glm::fvec3), nullptr);
    glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 6);
}

void debug_draw::line(const EntityPos &start, const glm::fvec3 &scale, float width, const glm::fvec3 &color)
{
    EntityPos patch = start;
    patch.chunk -= camera::position().chunk;

    glLineWidth(width);
    
    glUniform3fv(u_world_position, 1, glm::value_ptr(coord::to_world(patch)));
    glUniform3fv(u_scale, 1, glm::value_ptr(scale));
    glUniform3fv(u_color, 1, glm::value_ptr(color));

    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(glm::fvec3), nullptr);
    glDrawArrays(GL_LINES, 0, 2);
}
