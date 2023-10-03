// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gl_program.hh>
#include <client/gl_sampler.hh>
#include <client/gl_vertexarray.hh>
#include <client/label.hh>
#include <client/screen.hh> // FIXME
#include <client/shaders.hh>
#include <spdlog/spdlog.h>
#include <vector>

struct LabelRender_UBO final {
    vector4f_t screen {};
    vector4f_t glyph {};
    vector4f_t color {};
    vector4f_t rect {};
};

static gl::Buffer ubo = {};
static gl::Program program = {};
static gl::Sampler sampler = {};
static gl::VertexArray vao = {};

void Label::set_text(const std::wstring &text)
{
    std::vector<uint32_t> pixels = {};

    text_size = text.size();
    pixels.resize(text_size);

    for(unsigned int i = 0; i < text_size; ++i) {
        // FIXME: this assumes std::wstring is
        // either encoded as UTF-32 or as UTF-16
        // without surrogates. This is potentially
        // bad for, say a whole plane of CJK characters
        pixels[i] = static_cast<uint32_t>(text[i]);
    }

    texture.create();
    texture.storage(text_size, 1, PixelFormat::R32_UINT);
    texture.write(0, 0, text_size, 1, PixelFormat::R32_UINT, pixels.data());
}

void Label::set_color(const vector4f_t &color)
{
    this->color = color;
}

void Label::set_position(const vector2u_t &position)
{
    this->position = position;
}

void Label::set_scale(const vector2f_t &scale)
{
    this->scale = scale;
}

void Label::destroy()
{
    text_size = 0;
    color = vector4f_t{1.0, 1.0, 1.0, 1.0};
    position = vector2u_t{};
    scale = vector2f_t{1.0, 1.0};
    texture.destroy();
}

void Label::init()
{
    ubo.create();
    ubo.storage(sizeof(LabelRender_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    gl::Shader vert = {};
    gl::Shader frag = {};
    std::string source = {};

    vert.create(GL_VERTEX_SHADER);
    frag.create(GL_FRAGMENT_SHADER);

    if(!shaders::compile(vert, "/shaders/label.vert"))
        std::terminate();
    if(!shaders::compile(frag, "/shaders/label.frag"))
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

void Label::deinit()
{
    vao.destroy();
    sampler.destroy();
    program.destroy();
    ubo.destroy();
}

void Label::draw(const Label &label, const Font *font)
{
    if(font != nullptr) {
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        font->texture.bind(0);
        label.texture.bind(1);

        sampler.bind(0);
        sampler.bind(1);

        // FIXME
        int width, height;
        screen::get_size(width, height);
        glViewport(0, 0, width, height);

        LabelRender_UBO uniforms = {};
        uniforms.screen.x = width;
        uniforms.screen.y = height;
        uniforms.screen.z = 1.0 / uniforms.screen.x;
        uniforms.screen.w = 1.0 / uniforms.screen.y;
        uniforms.glyph.x = label.scale.x * font->glyph_width;
        uniforms.glyph.y = label.scale.y * font->glyph_height;
        uniforms.glyph.z = font->texture_cwidth;
        uniforms.glyph.w = font->texture_cheight;
        uniforms.color = label.color;
        uniforms.rect.x = label.position.x;
        uniforms.rect.y = label.position.y;
        uniforms.rect.z = uniforms.glyph.x * label.text_size;
        uniforms.rect.w = uniforms.glyph.y;

        ubo.bind_base(GL_UNIFORM_BUFFER, 0);
        ubo.write(0, sizeof(uniforms), &uniforms);

        vao.bind();
        program.bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}
