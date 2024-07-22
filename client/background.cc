// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/background.hh>
#include <client/globals.hh>
#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <shared/const.hh>
#include <spdlog/spdlog.h>

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

    GLuint vert = util::compile_shader("/shaders/background.vert", GL_VERTEX_SHADER);
    GLuint frag = util::compile_shader("/shaders/background.frag", GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        spdlog::critical("background: shader compile failed");
        std::terminate();
    }

    bg_program = util::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!bg_program) {
        spdlog::critical("background: program link failed");
        std::terminate();
    }

    u_time = glGetUniformLocation(bg_program, "u_Time");

    const glm::fvec2 vertices[6] = {
        glm::fvec2(-1.0f, -1.0f),
        glm::fvec2(-1.0f,  1.0f),
        glm::fvec2( 1.0f,  1.0f),

        glm::fvec2( 1.0f,  1.0f),
        glm::fvec2( 1.0f, -1.0f),
        glm::fvec2(-1.0f, -1.0f),
    };

    glGenVertexArrays(1, &bg_vaobj);
    glBindVertexArray(bg_vaobj);

    glGenBuffers(1, &bg_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, bg_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(glm::fvec2), nullptr);
}

void background::deinit(void)
{
    glDeleteBuffers(1, &bg_vbo);
    glDeleteVertexArrays(1, &bg_vaobj);
    glDeleteProgram(bg_program);
}

void background::render(void)
{
    const float curtime = glfwGetTime();
    glUseProgram(bg_program);
    glUniform1fv(u_time, 1, &curtime);
    glBindVertexArray(bg_vaobj);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
