// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/globals.hh>
#include <client/glxx/program.hh>
#include <client/glxx/sampler.hh>
#include <client/glxx/vertexarray.hh>
#include <client/shaders.hh>
#include <shared/color.hh>

struct Canvas_UBO final {
    vector4f_t col_ul {COL_TRANSPARENT}; // top left or foreground
    vector4f_t col_ur {COL_TRANSPARENT}; // top right or background
    vector4f_t col_dl {COL_TRANSPARENT}; // bottom left
    vector4f_t col_dr {COL_TRANSPARENT}; // bottom right
    vector4f_t screen {};
    vector4f_t glyph {};
    vector4f_t rect {};
};

static glxx::Buffer ubo = {};
static glxx::Program program_rect_col = {};
static glxx::Program program_rect_tex = {};
static glxx::Program program_text = {};
static glxx::Sampler sampler = {};
static glxx::VertexArray vao = {};

static void init_program(glxx::Program &prog, const vfs::path_t &fpath, const glxx::Shader &vert)
{
    glxx::Shader frag = {};
    frag.create(GL_FRAGMENT_SHADER);

    if(!shaders::compile(frag, fpath))
        std::terminate();

    prog.create();
    prog.attach(vert);
    prog.attach(frag);

    if(!prog.link())
        std::terminate();
    frag.destroy();
}

void canvas::init()
{
    ubo.create();
    ubo.storage(sizeof(Canvas_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    glxx::Shader vert = {};
    vert.create(GL_VERTEX_SHADER);

    if(!shaders::compile(vert, "/shaders/canvas_common.vert"))
        std::terminate();
    init_program(program_rect_col, "/shaders/canvas_rect_col.frag", vert);
    init_program(program_rect_tex, "/shaders/canvas_rect_tex.frag", vert);
    init_program(program_text, "/shaders/canvas_text.frag", vert);

    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    vao.create();
}

void canvas::deinit()
{
    vao.destroy();
    sampler.destroy();
    program_rect_tex.destroy();
    program_rect_col.destroy();
    program_text.destroy();
    ubo.destroy();
}

void canvas::draw_rect(int xpos, int ypos, int width, int height)
{
    canvas::draw_rect(xpos, ypos, width, height, COL_WHITE);
}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const vector4_t &color)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = color;
    uniforms.col_ur = color;
    uniforms.col_dl = color;
    uniforms.col_dr = color;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    // FIXME: bind it once and then just draw?
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const glxx::Texture2D &texture)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = COL_WHITE;
    uniforms.col_ur = COL_WHITE;
    uniforms.col_dl = COL_WHITE;
    uniforms.col_dr = COL_WHITE;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    texture.bind(0);

    sampler.bind(0);

    // FIXME: bind it once and then just draw?
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();

    program_rect_tex.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const vector4_t &color, const glxx::Texture2D &texture)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = color;
    uniforms.col_ur = color;
    uniforms.col_dl = color;
    uniforms.col_dr = color;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    texture.bind(0);

    sampler.bind(0);

    // FIXME: bind it once and then just draw?
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();

    program_rect_tex.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void canvas::draw_rect_h(int xpos, int ypos, int width, int height, const vector4_t &colx, const vector4_t &coly)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = colx;
    uniforms.col_ur = coly;
    uniforms.col_dl = colx;
    uniforms.col_dr = coly;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    // FIXME: bind it once and then just draw?
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void canvas::draw_rect_v(int xpos, int ypos, int width, int height, const vector4_t &colx, const vector4_t &coly)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = colx;
    uniforms.col_ur = colx;
    uniforms.col_dl = coly;
    uniforms.col_dr = coly;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    // FIXME: bind it once and then just draw?
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font)
{
    canvas::draw_text(xpos, ypos, text, font, COL_WHITE, COL_TRANSPARENT, 1U);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, unsigned int scale)
{
    canvas::draw_text(xpos, ypos, text, font, COL_WHITE, COL_TRANSPARENT, scale);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const vector4_t &fg)
{
    canvas::draw_text(xpos, ypos, text, font, fg, COL_TRANSPARENT, 1U);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const vector4_t &fg, unsigned int scale)
{
    canvas::draw_text(xpos, ypos, text, font, fg, COL_TRANSPARENT, scale);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const vector4_t &fg, const vector4_t &bg)
{
    canvas::draw_text(xpos, ypos, text, font, fg, bg, 1U);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const vector4_t &fg, const vector4_t &bg, unsigned int scale)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = fg;
    uniforms.col_ur = bg;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width() * static_cast<double>(scale);
    uniforms.glyph.y = font.get_glyph_height() * static_cast<double>(scale);
    uniforms.glyph.z = font.get_texture_cwidth();
    uniforms.glyph.w = font.get_texture_cheight();
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = uniforms.glyph.x * text.get_texture_width();
    uniforms.rect.w = uniforms.glyph.y * text.get_texture_height();

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    font.get().bind(0);
    text.get().bind(1);

    sampler.bind(0);
    sampler.bind(1);

    // FIXME: bind it once and then just draw?
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();

    program_text.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
