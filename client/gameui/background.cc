// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gameui/background.hh>
#include <client/globals.hh>
#include <client/util/shadertools.hh>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <shared/math/vector2D.hh>

static GLint u_time = {};
static GLuint bg_program = {};
static GLuint bg_vaobj = {};
static GLuint bg_vbo = {};

void background::init(void)
{
    u_time = 0;
    bg_program = 0;
    bg_vaobj = 0;
    bg_vbo = 0;

    GLuint vert = shadertools::compile_shader("shaders/background.vert", GL_VERTEX_SHADER);
    GLuint frag = shadertools::compile_shader("shaders/background.frag", GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        spdlog::critical("background: shader compile failed");
        std::terminate();
    }

    bg_program = shadertools::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!bg_program) {
        spdlog::critical("background: program link failed");
        std::terminate();
    }

    u_time = glGetUniformLocation(bg_program, "u_Time");

    const Vector2D vertices[4] = {
        Vector2D(-1.0f,  1.0f),
        Vector2D(-1.0f, -1.0f),
        Vector2D( 1.0f,  1.0f),
        Vector2D( 1.0f, -1.0f),
    };

    glGenVertexArrays(1, &bg_vaobj);
    glBindVertexArray(bg_vaobj);

    glGenBuffers(1, &bg_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, bg_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vector2D), nullptr);
}

void background::deinit(void)
{
    glDeleteBuffers(1, &bg_vbo);
    glDeleteVertexArrays(1, &bg_vaobj);
    glDeleteProgram(bg_program);
}

void background::render(void)
{
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(bg_program);
    glUniform1f(u_time, glfwGetTime());

    glBindVertexArray(bg_vaobj);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
