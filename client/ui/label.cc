// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gl/program.hh>
#include <client/gl/sampler.hh>
#include <client/gl/vertexarray.hh>
#include <client/globals.hh>
#include <client/shaders.hh>
#include <client/ui/label.hh>
#include <vector>

struct LabelDraw_UBO final {
    vector4f_t screen {};
    vector4f_t glyph {};
    vector4f_t color {};
    vector4f_t rect {};
};

static gl::Buffer ubo = {};
static gl::Program program = {};
static gl::Sampler sampler = {};
static gl::VertexArray vao = {};

void ui::Label::set_text(const std::wstring &text)
{
    std::vector<uint32_t> pixels = {};
    const int last_width = texture_width;

    texture_width = text.size();
    pixels.resize(texture_width);

    for(int i = 0; i < texture_width; ++i) {
        // NOTE: this assumes the string contains UTF-16
        // without surrogates or simply a pure UTF-32.
        pixels[i] = static_cast<uint32_t>(text[i]);
    }

    if(!texture.get() || (texture_width > last_width)) {
        texture.create();
        texture.storage(texture_width, 1, PixelFormat::R32_UINT);
    }

    texture.write(0, 0, texture_width, 1, PixelFormat::R32_UINT, pixels.data());
}

void ui::Label::set_center(const vector2i_t &center)
{
    this->center = center;
}

void ui::Label::set_color(const vector4_t &color)
{
    this->color = color;
}

void ui::Label::set_scale(const vector2_t &scale)
{
    this->scale = scale;
}

void ui::Label::destroy()
{
    texture_width = 0;
    center = vector2i_t{};
    color = {1.0, 1.0, 1.0, 1.0};
    scale = {1.0, 1.0};
    texture.destroy();
}

void ui::Label::init()
{
    ubo.create();
    ubo.storage(sizeof(LabelDraw_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    gl::Shader vert = {};
    gl::Shader frag = {};

    vert.create(GL_VERTEX_SHADER);
    frag.create(GL_FRAGMENT_SHADER);

    if(!shaders::compile(vert, "/shaders/ui_label.vert"))
        std::terminate();
    if(!shaders::compile(frag, "/shaders/ui_label.frag"))
        std::terminate();

    program.create();
    program.attach(vert);
    program.attach(frag);

    if(!program.link())
        std::terminate();
    vert.destroy();
    frag.destroy();

    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    vao.create();
}

void ui::Label::deinit()
{
    vao.destroy();
    sampler.destroy();
    program.destroy();
    ubo.destroy();
}

void ui::Label::draw(const ui::Label &label, const ui::Font &font)
{
    glViewport(0, 0, globals::window_width, globals::window_height);

    font.get().bind(0);
    label.texture.bind(1);

    sampler.bind(0);
    sampler.bind(1);

    const double scale_xy = globals::ui_scale;

    LabelDraw_UBO uniforms = {};
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = label.scale.x * scale_xy * font.get_glyph_width();
    uniforms.glyph.y = label.scale.y * scale_xy * font.get_glyph_height();
    uniforms.glyph.z = font.get_texture_cwidth();
    uniforms.glyph.w = font.get_texture_cheight();
    uniforms.color = label.color;
    uniforms.rect.x = scale_xy * label.center.x;
    uniforms.rect.y = scale_xy * label.center.y;
    uniforms.rect.z = uniforms.glyph.x * label.texture_width;
    uniforms.rect.w = uniforms.glyph.y;

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    ubo.write(0, sizeof(LabelDraw_UBO), &uniforms);

    vao.bind();
    program.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

