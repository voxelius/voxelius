// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/background.hh>
#include <client/globals.hh>
#include <client/varied_program.hh>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <shared/math/vec2f.hh>

static VariedProgram program = {};
static std::size_t u_time = {};
static GLuint vaobj = {};
static GLuint vbo = {};

void background::init(void)
{
    if(!VariedProgram::setup(program, "shaders/background.vert", "shaders/background.frag")) {
        spdlog::critical("background: program setup failed");
        std::terminate();
    }
    
    u_time = VariedProgram::uniform(program, "u_Time");

    const Vec2f vertices[4] = {
        Vec2f(-1.0f,  1.0f),
        Vec2f(-1.0f, -1.0f),
        Vec2f( 1.0f,  1.0f),
        Vec2f( 1.0f, -1.0f),
    };

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vec2f), nullptr);
}

void background::deinit(void)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vaobj);
    VariedProgram::destroy(program);
}

void background::render(void)
{
    if(!VariedProgram::update(program)) {
        spdlog::critical("background: program update failed");
        VariedProgram::destroy(program);
        std::terminate();
    }
    
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(program.handle);
    glUniform1f(program.uniforms[u_time].location, glfwGetTime());

    glBindVertexArray(vaobj);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
