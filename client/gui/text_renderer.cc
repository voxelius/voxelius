// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/bitmap_font.hh>
#include <client/gui/text_quad.hh>
#include <client/gui/text_renderer.hh>
#include <client/gui/text_vbo.hh>
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/globals.hh>
#include <shared/math/vec2f.hh>
#include <shared/math/vec4f.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

static GLuint program = {};
static GLuint vaobj = {};
static GLuint vbo = {};

static GLint u_screen_size = {};
static GLint u_offset = {};
static GLint u_glyph_size = {};
static GLint u_tint = {};
static GLint u_time = {};
static GLint u_font = {};

void text_renderer::init(void)
{
    GLuint vert = util::compile_shader("shaders/gui_text.vert", GL_VERTEX_SHADER);
    GLuint frag = util::compile_shader("shaders/gui_text.frag", GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        spdlog::critical("text_renderer: shader compile failed");
        std::terminate();
    }

    program = util::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!program) {
        spdlog::critical("text_renderer: program link failed");
        std::terminate();
    }

    const Vec2f vertices[4] = {
        Vec2f(1.0f, 1.0f),
        Vec2f(1.0f, 0.0f),
        Vec2f(0.0f, 1.0f),
        Vec2f(0.0f, 0.0f),
    };

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vec2f), nullptr);

    u_screen_size = glGetUniformLocation(program, "u_ScreenSize");
    u_offset = glGetUniformLocation(program, "u_Offset");
    u_glyph_size = glGetUniformLocation(program, "u_GlyphSize");
    u_tint = glGetUniformLocation(program, "u_Tint");
    u_time = glGetUniformLocation(program, "u_Time");
    u_font = glGetUniformLocation(program, "u_Font");
}

void text_renderer::deinit(void)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vaobj);
    glDeleteProgram(program);
}
#include <GLFW/glfw3.h>
void text_renderer::prepare(void)
{
    Vec4f screen_size = {};
    screen_size[0] = static_cast<float>(globals::width);
    screen_size[1] = static_cast<float>(globals::height);
    screen_size[2] = 1.0f / screen_size[0];
    screen_size[3] = 1.0f / screen_size[1];

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(program);
    glUniform4fv(u_screen_size, 1, screen_size.data());
    glUniform1f(u_time, glfwGetTime());

    glBindVertexArray(vaobj);
}

void text_renderer::draw(float xpos, float ypos, const BitmapFont &font, const TextVBO &text, float scale, bool shadow)
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, font.handle);
    glActiveTexture(GL_TEXTURE0);
    
    glUniform2f(u_offset, xpos, ypos);
    glUniform2f(u_glyph_size, font.glyph_height, scale);
    glUniform1f(u_tint, shadow ? 0.25f : 1.0f);
    glUniform1i(u_font, 0); // GL_TEXTURE0

    glBindBuffer(GL_ARRAY_BUFFER, text.handle);
    glVertexAttribIPointer(1, 4, GL_UNSIGNED_INT, sizeof(TextQuad), reinterpret_cast<void *>(offsetof(TextQuad, data_1)));
    glVertexAttribIPointer(2, 4, GL_UNSIGNED_INT, sizeof(TextQuad), reinterpret_cast<void *>(offsetof(TextQuad, data_2)));

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, text.size);
}
