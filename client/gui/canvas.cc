// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/bitmap_font.hh>
#include <client/gui/canvas.hh>
#include <client/gui/text_vbo.hh>
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/globals.hh>
#include <shared/math/vec2f.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

static bool rect_bound = false;
static GLuint rect_program = {};
static GLint rect_u_screen_size = {};
static GLint rect_u_offset = {};
static GLint rect_u_size = {};
static GLint rect_u_color = {};

static bool text_bound = false;
static GLuint text_program = {};
static GLint text_u_screen_size = {};
static GLint text_u_offset = {};
static GLint text_u_atlas_size = {};
static GLint text_u_glyph_size = {};
static GLint text_u_background = {};
static GLint text_u_foreground = {};
static GLint text_u_font = {};

static GLuint quad_vaobj = {};
static GLuint quad_vbo = {};

// These values are calculated when canvas::prepare
// is called and only passed into shaders when bound;
// This shaves off some of these CPU-to-GPU transfer times
static Vec4f screen_size = {};

static GLuint init_program(const std::string &name)
{
    GLuint vert = util::compile_shader(fmt::format("shaders/{}.vert", name), GL_VERTEX_SHADER);
    GLuint frag = util::compile_shader(fmt::format("shaders/{}.frag", name), GL_FRAGMENT_SHADER);
    
    if(!vert || !frag) {
        spdlog::critical("canvas: {}: shader compile failed", name);
        std::terminate();
    }
    
    GLuint program = util::link_program(vert, frag);
    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!program) {
        spdlog::critical("canvas: {}: program link failed", name);
        std::terminate();
    }

    return program;
}

void canvas::init(void)
{
    rect_bound = false;
    rect_program = init_program("canvas_rect");
    rect_u_screen_size = glGetUniformLocation(rect_program, "u_ScreenSize");
    rect_u_offset = glGetUniformLocation(rect_program, "u_Offset");
    rect_u_size = glGetUniformLocation(rect_program, "u_Size");
    rect_u_color = glGetUniformLocation(rect_program, "u_Color");

    text_bound = false;
    text_program = init_program("canvas_text");
    text_u_screen_size = glGetUniformLocation(text_program, "u_ScreenSize");
    text_u_offset = glGetUniformLocation(text_program, "u_Offset");
    text_u_atlas_size = glGetUniformLocation(text_program, "u_AtlasSize");
    text_u_glyph_size = glGetUniformLocation(text_program, "u_GlyphSize");
    text_u_background = glGetUniformLocation(text_program, "u_BackgroundColor");
    text_u_foreground = glGetUniformLocation(text_program, "u_ForegroundColor");
    text_u_font = glGetUniformLocation(text_program, "u_Font");

    // These are essentially UV coordinates; The shader's job
    // is to both convert them into NDC coordinates for each object
    // and scale and offset them in case we're rendering text
    const Vec2f vertices[4] = {
        Vec2f(1.0f, 1.0f),
        Vec2f(1.0f, 0.0f),
        Vec2f(0.0f, 1.0f),
        Vec2f(0.0f, 0.0f),
    };

    glGenVertexArrays(1, &quad_vaobj);
    glBindVertexArray(quad_vaobj);

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vec2f), nullptr);
}

void canvas::deinit(void)
{
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &quad_vaobj);
    glDeleteProgram(text_program);
    glDeleteProgram(rect_program);
}

void canvas::prepare(void)
{
    rect_bound = false;
    text_bound = false;
    
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

    glBindVertexArray(quad_vaobj);
}

void canvas::draw_rect(int xpos, int ypos, int width, int height)
{
    Vec4f rect_colors[4] = {};
    rect_colors[0] = Vec4f::white();
    rect_colors[1] = Vec4f::white();
    rect_colors[2] = Vec4f::white();
    rect_colors[3] = Vec4f::white();
    
    if(!rect_bound) {
        rect_bound = true;
        text_bound = false;
        glUseProgram(rect_program);
        glUniform4fv(rect_u_screen_size, 1, screen_size.data());
    }
    
    glUniform2f(rect_u_offset, xpos, ypos);
    glUniform2f(rect_u_size, width, height);
    glUniform4fv(rect_u_color, 4, rect_colors[0].data());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const Vec4f &color)
{
    Vec4f rect_colors[4] = {};
    rect_colors[0] = color;
    rect_colors[1] = color;
    rect_colors[2] = color;
    rect_colors[3] = color;
    
    if(!rect_bound) {
        rect_bound = true;
        text_bound = false;
        glUseProgram(rect_program);
        glUniform4fv(rect_u_screen_size, 1, screen_size.data());
    }
    
    glUniform2f(rect_u_offset, xpos, ypos);
    glUniform2f(rect_u_size, width, height);
    glUniform4fv(rect_u_color, 4, rect_colors[0].data());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_rect_h(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly)
{
    Vec4f rect_colors[4] = {};
    rect_colors[0] = colx;
    rect_colors[1] = colx;
    rect_colors[2] = coly;
    rect_colors[3] = coly;
    
    if(!rect_bound) {
        rect_bound = true;
        text_bound = false;
        glUseProgram(rect_program);
        glUniform4fv(rect_u_screen_size, 1, screen_size.data());
    }
    
    glUniform2f(rect_u_offset, xpos, ypos);
    glUniform2f(rect_u_size, width, height);
    glUniform4fv(rect_u_color, 4, rect_colors[0].data());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_rect_v(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly)
{
    Vec4f rect_colors[4] = {};
    rect_colors[0] = colx;
    rect_colors[1] = coly;
    rect_colors[2] = colx;
    rect_colors[3] = coly;
    
    if(!rect_bound) {
        rect_bound = true;
        text_bound = false;
        glUseProgram(rect_program);
        glUniform4fv(rect_u_screen_size, 1, screen_size.data());
    }
    
    glUniform2f(rect_u_offset, xpos, ypos);
    glUniform2f(rect_u_size, width, height);
    glUniform4fv(rect_u_color, 4, rect_colors[0].data());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text)
{
    canvas::draw_text(xpos, ypos, font, text, Vec4f::white(), Vec4f::transparent(), 1.0f);
}
void canvas::draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, float scale)
{
    canvas::draw_text(xpos, ypos, font, text, Vec4f::white(), Vec4f::transparent(), scale);
}

void canvas::draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg)
{
    canvas::draw_text(xpos, ypos, font, text, fg, Vec4f::transparent(), 1.0f);
}

void canvas::draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg, float scale)
{
    canvas::draw_text(xpos, ypos, font, text, fg, Vec4f::transparent(), scale);
}

void canvas::draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg, const Vec4f &bg)
{
    canvas::draw_text(xpos, ypos, font, text, fg, bg, 1.0f);
}

void canvas::draw_text(int xpos, int ypos, const BitmapFont &font, const TextVBO &text, const Vec4f &fg, const Vec4f &bg, float scale)
{
    if(!text_bound) {
        text_bound = true;
        rect_bound = false;
        glUseProgram(text_program);
        glUniform4fv(text_u_screen_size, 1, screen_size.data());
    }

    glBindTexture(GL_TEXTURE_2D, font.handle);
    glActiveTexture(GL_TEXTURE0);
    
    glUniform2f(text_u_offset, xpos, ypos);
    glUniform2f(text_u_atlas_size, font.num_glyphs_x, font.num_glyphs_y);
    glUniform3f(text_u_glyph_size, font.glyph_width, font.glyph_height, scale);
    glUniform4fv(text_u_background, 1, bg.data());
    glUniform4fv(text_u_foreground, 1, fg.data());
    glUniform1i(text_u_font, 0); // GL_TEXTURE0

    glBindBuffer(GL_ARRAY_BUFFER, text.handle);
    
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);
    glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, sizeof(TextQuad), nullptr);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, text.size);
}
