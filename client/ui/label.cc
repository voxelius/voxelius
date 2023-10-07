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
#include <sstream>
#include <vector>

struct LabelDraw_UBO final {
    vector4f_t background {};
    vector4f_t foreground {};
    vector4f_t screen {};
    vector4f_t glyph {};
    vector4f_t rect {};
};

static gl::Buffer ubo = {};
static gl::Program program = {};
static gl::Sampler sampler = {};
static gl::VertexArray vao = {};

void ui::Label::create(int width, int height)
{
    this->width = 1 << cxmath::log2(width);
    this->height = 1 << cxmath::log2(height);

    // We need to fill the texture up
    // with 0xFFFFFFFF which is a magic
    // constant for an invisible character
    std::vector<uint32_t> pixels = {};
    pixels.resize(this->width * this->height, 0xFFFFFFFF);

    texture.create();
    texture.storage(this->width, this->height, PixelFormat::R32_UINT);
    texture.write(0, 0, this->width, this->height, PixelFormat::R32_UINT, pixels.data());
}

void ui::Label::destroy()
{
    width = 0;
    height = 0;
    texture.destroy();
}

void ui::Label::set_text(int line, const std::string &text)
{
    std::vector<uint32_t> pixels = {};
    pixels.resize(width, 0xFFFFFFFF);

    // We only can write up to <width> characters
    const size_t size = cxmath::min<size_t>(text.size(), width);

    for(size_t i = 0; i < size; ++i) {
        // NOTE: this assumes the input string contains UTF-8
        // without multubyte sequences, practically ASCII.
        pixels[i] = static_cast<uint32_t>(text[i]);
    }

    texture.write(0, height - line - 1, width, 1, PixelFormat::R32_UINT, pixels.data());
}

void ui::Label::set_text(int line, const std::wstring &text)
{
    std::vector<uint32_t> pixels = {};
    pixels.resize(width, 0xFFFFFFFF);

    // We only can write up to <width> characters
    const size_t size = cxmath::min<size_t>(text.size(), width);

    for(size_t i = 0; i < size; ++i) {
        // NOTE: this assumes the input string contains
        // UTF-16 without surrogates or just pure UTF-32.
        pixels[i] = static_cast<uint32_t>(text[i]);
    }

    texture.write(0, height - line - 1, width, 1, PixelFormat::R32_UINT, pixels.data());
}

void ui::Label::set_background(const vector4_t &value)
{
    background = value;
}

void ui::Label::set_foreground(const vector4_t &value)
{
    foreground = value;
}

void ui::Label::set_position(const vector2i_t &value)
{
    position = value;
}

void ui::Label::set_scale(unsigned int value)
{
    scale = cxmath::max(1U, value);
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

void ui::Label::draw(const Label &label, const Font &font)
{
    glViewport(0, 0, globals::window_width, globals::window_height);

    font.get().bind(0);
    label.texture.bind(1);

    sampler.bind(0);
    sampler.bind(1);

    LabelDraw_UBO uniforms = {};
    uniforms.background = label.background;
    uniforms.foreground = label.foreground;
    uniforms.screen.x = globals::window_width;
    uniforms.screen.y = globals::window_height;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.glyph.x = font.get_glyph_width() * label.scale;
    uniforms.glyph.y = font.get_glyph_height() * label.scale;
    uniforms.glyph.z = font.get_texture_cwidth();
    uniforms.glyph.w = font.get_texture_cheight();
    uniforms.rect.x = label.position.x * label.scale;
    uniforms.rect.y = label.position.y * label.scale;
    uniforms.rect.z = uniforms.glyph.x * label.width;
    uniforms.rect.w = uniforms.glyph.y * label.height;

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    ubo.write(0, sizeof(LabelDraw_UBO), &uniforms);

    vao.bind();
    program.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

