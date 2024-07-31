// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/canvas/canvas.hh>
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/globals.hh>
#include <shared/math/vec2f.hh>
#include <spdlog/spdlog.h>

static GLuint program_rc = {};
static GLint u_rc_color_0 = {};
static GLint u_rc_color_1 = {};
static GLint u_rc_color_2 = {};
static GLint u_rc_color_3 = {};
static GLint u_rc_screen = {};
static GLint u_rc_rect = {};

static GLuint program_rt = {};
static GLint u_rt_color = {};
static GLint u_rt_screen = {};
static GLint u_rt_rect = {};
static GLint u_rt_texture = {};

static GLuint program_txt = {};
static GLint u_txt_color_bg = {};
static GLint u_txt_color_fg = {};
static GLint u_txt_screen = {};
static GLint u_txt_rect = {};
static GLint u_txt_glyph = {};
static GLint u_txt_font = {};
static GLint u_txt_text = {};

static GLuint vaobj = {};
static GLuint vbo = {};

static Vec4f u_screen_value = {};

static GLuint setup_program(const std::string &frag_path, GLuint vert)
{
    GLuint frag = util::compile_shader(frag_path, GL_FRAGMENT_SHADER);

    if(!frag) {
        spdlog::critical("canvas: {}: compile failed", frag_path);
        std::terminate();
    }

    GLuint program = util::link_program(vert, frag);

    glDeleteShader(frag);

    if(!program) {
        spdlog::critical("canvas: program link failed");
        std::terminate();
    }

    return program;
}

void canvas::init(void)
{
    const std::string vert_path = std::string("shaders/canvas_common.vert");

    GLuint vert = util::compile_shader(vert_path, GL_VERTEX_SHADER);

    if(!vert) {
        spdlog::critical("canvas: {}: compile failed", vert_path);
        std::terminate();
    }

    program_rc = setup_program("shaders/canvas_rect_colored.frag", vert);
    u_rc_color_0 = glGetUniformLocation(program_rc, "u_Color0");
    u_rc_color_1 = glGetUniformLocation(program_rc, "u_Color1");
    u_rc_color_2 = glGetUniformLocation(program_rc, "u_Color2");
    u_rc_color_3 = glGetUniformLocation(program_rc, "u_Color3");
    u_rc_screen = glGetUniformLocation(program_rc, "u_Screen");
    u_rc_rect = glGetUniformLocation(program_rc, "u_Rect");

    program_rt = setup_program("shaders/canvas_rect_textured.frag", vert);
    u_rt_color = glGetUniformLocation(program_rt, "u_Color0");
    u_rt_screen = glGetUniformLocation(program_rt, "u_Screen");
    u_rt_rect = glGetUniformLocation(program_rt, "u_Rect");
    u_rt_texture = glGetUniformLocation(program_rc, "u_Texture");
    
    program_txt = setup_program("shaders/canvas_text.frag", vert);
    u_txt_color_bg = glGetUniformLocation(program_txt, "u_Color0");
    u_txt_color_fg = glGetUniformLocation(program_txt, "u_Color1");
    u_txt_screen = glGetUniformLocation(program_txt, "u_Screen");
    u_txt_rect = glGetUniformLocation(program_txt, "u_Rect");
    u_txt_glyph = glGetUniformLocation(program_txt, "u_Glyph");
    u_txt_font = glGetUniformLocation(program_txt, "u_Font");
    u_txt_text = glGetUniformLocation(program_txt, "u_Font");

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
    glDeleteProgram(program_txt);
    glDeleteProgram(program_rt);
    glDeleteProgram(program_rc);
}

void canvas::prepare(void)
{
    u_screen_value[0] = globals::width;
    u_screen_value[1] = globals::height;
    u_screen_value[2] = 1.0f / u_screen_value[0];
    u_screen_value[3] = 1.0f / u_screen_value[1];
    // https://github.com/voxelius/voxelius/blob/be20b212480d0d17c25c51e3f337384a75707481/client/canvas.cc
}

void canvas::draw_rect(int xpos, int ypos, int width, int height)
{

}

void canvas::draw_rect(int xpos, int ypos, int width, int height, GLuint texture)
{

}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const Vec4f &color)
{

}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const Vec4f &color, GLuint texture)
{

}

void canvas::draw_rect_h(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly)
{

}

void canvas::draw_rect_v(int xpos, int ypos, int width, int height, const Vec4f &colx, const Vec4f &coly)
{

}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font)
{

}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, float scale)
{

}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg)
{

}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, float scale)
{

}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, const Vec4f &bg)
{

}

void canvas::draw_text(int xpos, int ypos, const Text &text, const Font &font, const Vec4f &fg, const Vec4f &bg, float scale)
{

}
