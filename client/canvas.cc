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

struct Canvas_UBO final {
    glm::fvec4 col_ul {COLOR_TRANSPARENT}; // top left or foreground
    glm::fvec4 col_ur {COLOR_TRANSPARENT}; // top right or background
    glm::fvec4 col_dl {COLOR_TRANSPARENT}; // bottom left
    glm::fvec4 col_dr {COLOR_TRANSPARENT}; // bottom right
    glm::fvec4 screen {};
    glm::fvec4 glyph {};
    glm::fvec4 rect {};
};

static glxx::Buffer ubo = {};
static glxx::Program program_rect_col = {};
static glxx::Program program_rect_tex = {};
static glxx::Program program_text = {};
static glxx::Sampler sampler = {};
static glxx::VertexArray vao = {};
static Canvas_UBO uniforms = {};

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

void canvas::prepare()
{
    uniforms.screen.x = globals::width;
    uniforms.screen.y = globals::height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    vao.bind();
}

void canvas::draw_rect(int xpos, int ypos, int width, int height)
{
    canvas::draw_rect(xpos, ypos, width, height, COLOR_WHITE);
}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const glm::dvec4 &color)
{
    uniforms.col_ul = color;
    uniforms.col_ur = color;
    uniforms.col_dl = color;
    uniforms.col_dr = color;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    globals::vertices_drawn += 6;
}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const glxx::Texture2D &texture)
{
    uniforms.col_ul = COLOR_WHITE;
    uniforms.col_ur = COLOR_WHITE;
    uniforms.col_dl = COLOR_WHITE;
    uniforms.col_dr = COLOR_WHITE;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    texture.bind(0);
    sampler.bind(0);

    program_rect_tex.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    globals::vertices_drawn += 6;
}

void canvas::draw_rect(int xpos, int ypos, int width, int height, const glm::dvec4 &color, const glxx::Texture2D &texture)
{
    uniforms.col_ul = color;
    uniforms.col_ur = color;
    uniforms.col_dl = color;
    uniforms.col_dr = color;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    texture.bind(0);
    sampler.bind(0);

    program_rect_tex.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    globals::vertices_drawn += 6;
}

void canvas::draw_rect_h(int xpos, int ypos, int width, int height, const glm::dvec4 &colx, const glm::dvec4 &coly)
{
    uniforms.col_ul = colx;
    uniforms.col_ur = coly;
    uniforms.col_dl = colx;
    uniforms.col_dr = coly;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    globals::vertices_drawn += 6;
}

void canvas::draw_rect_v(int xpos, int ypos, int width, int height, const glm::dvec4 &colx, const glm::dvec4 &coly)
{
    uniforms.col_ul = colx;
    uniforms.col_ur = colx;
    uniforms.col_dl = coly;
    uniforms.col_dr = coly;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.write(0, sizeof(Canvas_UBO), &uniforms);

    program_rect_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    globals::vertices_drawn += 6;
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font)
{
    canvas::draw_text(xpos, ypos, text, font, COLOR_WHITE, COLOR_TRANSPARENT, 1U);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, unsigned int scale)
{
    canvas::draw_text(xpos, ypos, text, font, COLOR_WHITE, COLOR_TRANSPARENT, scale);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const glm::dvec4 &fg)
{
    canvas::draw_text(xpos, ypos, text, font, fg, COLOR_TRANSPARENT, 1U);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const glm::dvec4 &fg, unsigned int scale)
{
    canvas::draw_text(xpos, ypos, text, font, fg, COLOR_TRANSPARENT, scale);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const glm::dvec4 &fg, const glm::dvec4 &bg)
{
    canvas::draw_text(xpos, ypos, text, font, fg, bg, 1U);
}

void canvas::draw_text(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const glm::dvec4 &fg, const glm::dvec4 &bg, unsigned int scale)
{
    uniforms.col_ul = fg;
    uniforms.col_ur = bg;
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

    program_text.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    globals::vertices_drawn += 6;
}
