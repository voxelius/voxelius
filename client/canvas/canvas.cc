// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/canvas/canvas.hh>
#include <client/canvas/font.hh>
#include <client/canvas/text.hh>
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/globals.hh>
#include <shared/math/vec2f.hh>
#include <spdlog/spdlog.h>

struct Pipeline final {
    GLuint program {};
    GLint u_color0 {};
    GLint u_color1 {};
    GLint u_color2 {};
    GLint u_color3 {};
    GLint u_screen {};
    GLint u_glyph {};
    GLint u_rect {};
};

static Pipeline pipeline_rect_col = {};
static Pipeline pipeline_rect_tex = {};
static Pipeline pipeline_text = {};
static Vec4f screen_value = {};
static GLuint vaobj = {};
static GLuint vbo = {};

static void init_pipeline(Pipeline &pipeline, const std::string &frag_path, GLuint vert)
{
    GLuint frag = util::compile_shader(frag_path, GL_FRAGMENT_SHADER);

    if(!frag) {
        spdlog::critical("canvas: {}: compile failed", frag_path);
        std::terminate();
    }

    pipeline.program = util::link_program(vert, frag);

    glDeleteShader(frag);

    if(!pipeline.program) {
        spdlog::critical("canvas: program link failed");
        std::terminate();
    }

    pipeline.u_color0 = glGetUniformLocation(pipeline.program, "u_Color0");
    pipeline.u_color1 = glGetUniformLocation(pipeline.program, "u_Color1");
    pipeline.u_color2 = glGetUniformLocation(pipeline.program, "u_Color2");
    pipeline.u_color3 = glGetUniformLocation(pipeline.program, "u_Color3");
    pipeline.u_screen = glGetUniformLocation(pipeline.program, "u_Screen");
    pipeline.u_glyph = glGetUniformLocation(pipeline.program, "u_Glyph");
    pipeline.u_rect = glGetUniformLocation(pipeline.program, "u_Rect");

}

void canvas::init(void)
{
    const std::string vert_path = std::string("shaders/canvas_common.vert");

    GLuint vert = util::compile_shader(vert_path, GL_VERTEX_SHADER);

    if(!vert) {
        spdlog::critical("canvas: {}: compile failed", vert_path);
        std::terminate();
    }

    init_pipeline(pipeline_rect_col, "shaders/canvas_rect_col.frag", vert);
    init_pipeline(pipeline_rect_tex, "shaders/canvas_rect_tex.frag", vert);
    init_pipeline(pipeline_text, "shaders/canvas_text.frag", vert);

    glDeleteShader(vert);

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

void canvas::deinit(void)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vaobj);
    glDeleteProgram(pipeline_text.program);
    glDeleteProgram(pipeline_rect_tex.program);
    glDeleteProgram(pipeline_rect_col.program);
}

void canvas::prepare(void)
{
    screen_value[0] = globals::width;
    screen_value[1] = globals::height;
    screen_value[2] = 1.0f / screen_value[0];
    screen_value[3] = 1.0f / screen_value[1];
    glBindVertexArray(vaobj);
}

void canvas::draw_rect(int xpos, int ypos, int wide, int tall)
{
    glUseProgram(pipeline_rect_col.program);
    glUniform4fv(pipeline_rect_col.u_color0, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_col.u_color1, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_col.u_color2, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_col.u_color3, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_col.u_screen, 1, screen_value.data());
    glUniform4fv(pipeline_rect_col.u_rect, 1, Vec4f(xpos, ypos, wide, tall).data());
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_rect(int xpos, int ypos, int wide, int tall, const Vec4f &color)
{
    glUseProgram(pipeline_rect_col.program);
    glUniform4fv(pipeline_rect_col.u_color0, 1, color.data());
    glUniform4fv(pipeline_rect_col.u_color1, 1, color.data());
    glUniform4fv(pipeline_rect_col.u_color2, 1, color.data());
    glUniform4fv(pipeline_rect_col.u_color3, 1, color.data());
    glUniform4fv(pipeline_rect_col.u_screen, 1, screen_value.data());
    glUniform4fv(pipeline_rect_col.u_rect, 1, Vec4f(xpos, ypos, wide, tall).data());
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_rect(int xpos, int ypos, int wide, int tall, const Vec4f &color, GLuint texture)
{
    glUseProgram(pipeline_rect_tex.program);
    glUniform4fv(pipeline_rect_tex.u_color0, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_tex.u_color1, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_tex.u_color2, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_tex.u_color3, 1, Vec4f::white().data());
    glUniform4fv(pipeline_rect_tex.u_screen, 1, screen_value.data());
    glUniform4fv(pipeline_rect_tex.u_rect, 1, Vec4f(xpos, ypos, wide, tall).data());
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_rect_h(int xpos, int ypos, int wide, int tall, const Vec4f &acolor, const Vec4f &bcolor)
{
    glUseProgram(pipeline_rect_col.program);
    glUniform4fv(pipeline_rect_col.u_color0, 1, acolor.data());
    glUniform4fv(pipeline_rect_col.u_color1, 1, bcolor.data());
    glUniform4fv(pipeline_rect_col.u_color2, 1, acolor.data());
    glUniform4fv(pipeline_rect_col.u_color3, 1, bcolor.data());
    glUniform4fv(pipeline_rect_col.u_screen, 1, screen_value.data());
    glUniform4fv(pipeline_rect_col.u_rect, 1, Vec4f(xpos, ypos, wide, tall).data());
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_rect_v(int xpos, int ypos, int wide, int tall, const Vec4f &acolor, const Vec4f &bcolor)
{
    glUseProgram(pipeline_rect_col.program);
    glUniform4fv(pipeline_rect_col.u_color0, 1, acolor.data());
    glUniform4fv(pipeline_rect_col.u_color1, 1, acolor.data());
    glUniform4fv(pipeline_rect_col.u_color2, 1, bcolor.data());
    glUniform4fv(pipeline_rect_col.u_color3, 1, bcolor.data());
    glUniform4fv(pipeline_rect_col.u_screen, 1, screen_value.data());
    glUniform4fv(pipeline_rect_col.u_rect, 1, Vec4f(xpos, ypos, wide, tall).data());
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font)
{
    canvas::draw_text(xpos, ypos, text, font, Vec4f::white(), Vec4f::transparent(), 1.0f);
}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, float scale)
{
    canvas::draw_text(xpos, ypos, text, font, Vec4f::white(), Vec4f::transparent(), scale);
}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg)
{
    canvas::draw_text(xpos, ypos, text, font, fg, Vec4f::transparent(), 1.0f);
}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, float scale)
{
    canvas::draw_text(xpos, ypos, text, font, fg, Vec4f::transparent(), scale);
}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, const Vec4f &bg)
{
    canvas::draw_text(xpos, ypos, text, font, fg, bg, 1.0f);
}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, const Vec4f &bg, float scale)
{
    Vec4f glyph_value = {};
    glyph_value[0] = font.glyph_width * scale;
    glyph_value[1] = font.glyph_height * scale;
    glyph_value[2] = font.texture_x_glyphs;
    glyph_value[3] = font.texture_y_glyphs;

    Vec4f rect_value = {};
    rect_value[0] = xpos;
    rect_value[1] = ypos;
    rect_value[2] = glyph_value[0] * text.texture_width;
    rect_value[3] = glyph_value[1] * text.texture_width;

    glUseProgram(pipeline_text.program);
    glUniform4fv(pipeline_text.u_color0, 1, fg.data());
    glUniform4fv(pipeline_text.u_color1, 1, bg.data());
    glUniform4fv(pipeline_text.u_screen, 1, screen_value.data());
    glUniform4fv(pipeline_text.u_glyph, 1, Vec4f(font.glyph_width * scale, font.glyph_height * scale, font.texture_x_glyphs, font.texture_y_glyphs).data());
    glUniform4fv(pipeline_text.u_rect, 1, Vec4f(xpos, ypos, ))

    glUniform4fv(pipeline_rect_col.u_rect, 1, Vec4f(xpos, ypos, wide, tall).data());


}
