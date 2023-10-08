// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gl_program.hh>
#include <client/gl_sampler.hh>
#include <client/gl_vertexarray.hh>
#include <client/globals.hh>
#include <client/shaders.hh>
#include <client/ui_color.hh>
#include <client/ui_draw.hh>
#include <client/ui_font.hh>
#include <client/ui_text.hh>
#include <vector>

struct UI_Draw_UBO final {
    vector4f_t background {};
    vector4f_t foreground {};
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

void ui::draw::init()
{
    ubo.create();
    ubo.storage(sizeof(UI_Draw_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    gl::Shader vert = {};
    vert.create(GL_VERTEX_SHADER);

    if(!shaders::compile(vert, "/shaders/ui_common.vert"))
        std::terminate();
    init_program(program_text, "/shaders/ui_text.frag", vert);
    init_program(program_rect_col, "/shaders/ui_rect_col.frag", vert);
    init_program(program_rect_tex, "/shaders/ui_rect_tex.frag", vert);

    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    vao.create();
}

void ui::draw::deinit()
{
    vao.destroy();
    sampler.destroy();
    program_rect_tex.destroy();
    program_rect_col.destroy();
    program_text.destroy();
    ubo.destroy();
}

void ui::draw::prepare()
{
    glViewport(0, 0, globals::window_width, globals::window_height);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    sampler.bind(0);
    sampler.bind(1);

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);

    vao.bind();
}

void ui::draw::rect(int xpos, int ypos, int width, int height)
{
    ui::draw::rect(xpos, ypos, width, height, ui::WHITE);
}

void ui::draw::rect(int xpos, int ypos, int width, int height, const vector4_t &color)
{
    UI_Draw_UBO uniforms = {};
    uniforms.foreground = color;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(UI_Draw_UBO), &uniforms);

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ui::draw::rect(int xpos, int ypos, int width, int height, const gl::Texture2D &texture)
{
    ui::draw::rect(xpos, ypos, width, height, ui::WHITE, texture);
}

void ui::draw::rect(int xpos, int ypos, int width, int height, const vector4_t &color, const gl::Texture2D &texture)
{
    texture.bind(0);

    UI_Draw_UBO uniforms = {};
    uniforms.foreground = color;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(UI_Draw_UBO), &uniforms);

    program_rect_tex.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ui::draw::text(int xpos, int ypos, const ui::Text &text, const ui::Font &font)
{
    ui::draw::text(xpos, ypos, text, font, ui::WHITE, ui::TRANSPARENT, 1U);
}

void ui::draw::text(int xpos, int ypos, const ui::Text &text, const ui::Font &font, unsigned int scale)
{
    ui::draw::text(xpos, ypos, text, font, ui::WHITE, ui::TRANSPARENT, scale);
}

void ui::draw::text(int xpos, int ypos, const ui::Text &text, const ui::Font &font, const vector4_t &fg)
{
    ui::draw::text(xpos, ypos, text, font, fg, ui::TRANSPARENT, 1U);
}

void ui::draw::text(int xpos, int ypos, const ui::Text &text, const ui::Font &font, const vector4_t &fg, unsigned int scale)
{
    ui::draw::text(xpos, ypos, text, font, fg, ui::TRANSPARENT, scale);
}

void ui::draw::text(int xpos, int ypos, const ui::Text &text, const ui::Font &font, const vector4_t &fg, const vector4_t &bg)
{
    ui::draw::text(xpos, ypos, text, font, fg, bg, 1U);
}

void ui::draw::text(int xpos, int ypos, const ui::Text &text, const ui::Font &font, const vector4_t &fg, const vector4_t &bg, unsigned int scale)
{
    font.get().bind(0);
    text.get().bind(1);

    UI_Draw_UBO uniforms = {};
    uniforms.background = bg;
    uniforms.foreground = fg;
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

    ubo.write(0, sizeof(UI_Draw_UBO), &uniforms);

    program_text.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
