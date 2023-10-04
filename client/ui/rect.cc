// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gl/program.hh>
#include <client/gl/sampler.hh>
#include <client/gl/vertexarray.hh>
#include <client/globals.hh>
#include <client/shaders.hh>
#include <client/ui/rect.hh>

struct RectDraw_UBO final {
    vector4f_t screen {};
    vector4f_t color {};
    vector4f_t rect {};
};

static gl::Buffer ubo = {};
static gl::Program program_col = {};
static gl::Program program_tex = {};
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

void ui::Rect::set_center(const vector2i_t &center)
{
    this->center = center;
}

void ui::Rect::set_color(const vector4_t &color)
{
    this->color = color;
}

void ui::Rect::set_size(const vector2u_t &size)
{
    this->size = size;
}

void ui::Rect::init()
{
    ubo.create();
    ubo.storage(sizeof(RectDraw_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    gl::Shader vert = {};
    vert.create(GL_VERTEX_SHADER);

    if(!shaders::compile(vert, "/shaders/ui_rect.vert"))
        std::terminate();
    init_program(program_col, "/shaders/ui_rect_colored.frag", vert);
    init_program(program_tex, "/shaders/ui_rect_textured.frag", vert);

    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    vao.create();
}

void ui::Rect::deinit()
{
    vao.destroy();
    sampler.destroy();
    program_tex.destroy();
    program_col.destroy();
    ubo.destroy();
}

void ui::Rect::draw(const ui::Rect &rect)
{
    glViewport(0, 0, globals::window_width, globals::window_height);

    const double scale_xy = globals::ui_scale;

    RectDraw_UBO uniforms = {};
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.color = rect.color;
    uniforms.rect.x = scale_xy * rect.center.x;
    uniforms.rect.y = scale_xy * rect.center.y;
    uniforms.rect.z = scale_xy * rect.size.x;
    uniforms.rect.w = scale_xy * rect.size.y;

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    ubo.write(0, sizeof(RectDraw_UBO), &uniforms);

    vao.bind();
    program_col.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ui::Rect::draw(const ui::Rect &rect, const gl::Texture2D &texture)
{
    glViewport(0, 0, globals::window_width, globals::window_height);

    texture.bind(0);
    sampler.bind(0);

    const double scale_xy = globals::ui_scale;

    RectDraw_UBO uniforms = {};
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.color = rect.color;
    uniforms.rect.x = scale_xy * rect.center.x;
    uniforms.rect.y = scale_xy * rect.center.y;
    uniforms.rect.z = scale_xy * rect.size.x;
    uniforms.rect.w = scale_xy * rect.size.y;

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    ubo.write(0, sizeof(RectDraw_UBO), &uniforms);

    vao.bind();
    program_tex.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

