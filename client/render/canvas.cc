// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/render/gl/program.hh>
#include <client/render/gl/sampler.hh>
#include <client/render/gl/vertexarray.hh>
#include <client/render/canvas.hh>
#include <client/render/font.hh>
#include <client/render/text.hh>
#include <client/render/shaders.hh>
#include <client/globals.hh>
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

static gl::Buffer ubo = {};
static gl::Program program_text = {};
static gl::Program program_rect_col = {};
static gl::Program program_rect_tex = {};
static gl::Sampler sampler = {};
static gl::VertexArray vao = {};

static void init_program(gl::Program &prog, const vfs::path_t &fpath, const gl::Shader &vert)
{
    gl::Shader frag = {};
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

}

void canvas::deinit()
{

}

void canvas::rect(int xpos, int ypos, int width, int height)
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

    // FIXME: bind it once and then just draw?
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void canvas::rect(int xpos, int ypos, int width, int height, const vector4_t &color)
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

void canvas::rect(int xpos, int ypos, int width, int height, const gl::Texture2D &texture)
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

void canvas::rect(int xpos, int ypos, int width, int height, const vector4_t &color, const gl::Texture2D &texture)
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

void canvas::rect_h(int xpos, int ypos, int width, int height, const vector4_t &colx, const vector4_t &coly)
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

void canvas::rect_v(int xpos, int ypos, int width, int height, const vector4_t &colx, const vector4_t &coly)
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

void canvas::text(int xpos, int ypos, const Text &text, const Font &font)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = COL_WHITE;
    uniforms.col_ur = COL_TRANSPARENT;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width();
    uniforms.glyph.y = font.get_glyph_height();
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

void canvas::text(int xpos, int ypos, const Text &text, const Font &font, unsigned int scale)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = COL_WHITE;
    uniforms.col_ur = COL_TRANSPARENT;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width() * scale;
    uniforms.glyph.y = font.get_glyph_height() * scale;
    uniforms.glyph.z = font.get_texture_cwidth();
    uniforms.glyph.w = font.get_texture_cheight();
    uniforms.rect.x = xpos * scale;
    uniforms.rect.y = ypos * scale;
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

void canvas::text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = fg;
    uniforms.col_ur = COL_TRANSPARENT;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width();
    uniforms.glyph.y = font.get_glyph_height();
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

void canvas::text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, unsigned int scale)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = fg;
    uniforms.col_ur = COL_TRANSPARENT;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width() * scale;
    uniforms.glyph.y = font.get_glyph_height() * scale;
    uniforms.glyph.z = font.get_texture_cwidth();
    uniforms.glyph.w = font.get_texture_cheight();
    uniforms.rect.x = xpos * scale;
    uniforms.rect.y = ypos * scale;
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

void canvas::text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, const vector4_t &bg)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = fg;
    uniforms.col_ur = bg;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width();
    uniforms.glyph.y = font.get_glyph_height();
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

void canvas::text(int xpos, int ypos, const Text &text, const Font &font, const vector4_t &fg, const vector4_t &bg, unsigned int scale)
{
    Canvas_UBO uniforms = {};
    uniforms.col_ul = fg;
    uniforms.col_ur = bg;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width() * scale;
    uniforms.glyph.y = font.get_glyph_height() * scale;
    uniforms.glyph.z = font.get_texture_cwidth();
    uniforms.glyph.w = font.get_texture_cheight();
    uniforms.rect.x = xpos * scale;
    uniforms.rect.y = ypos * scale;
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
