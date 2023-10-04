// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gl_program.hh>
#include <client/gl_sampler.hh>
#include <client/gl_vertexarray.hh>
#include <client/screen.hh> // FIXME
#include <client/shaders.hh>
#include <client/ui_draw.hh>
#include <shared/cxmath.hh>
#include <spdlog/spdlog.h>

struct UiDraw_UBO final {
    vector4f_t screen {};
    vector4f_t glyph {};
    vector4f_t color {};
    vector4f_t rect {};
};

static gl::Buffer ubo = {};
static gl::Program program_label = {};
static gl::Program program_rect_colored = {};
static gl::Program program_rect_textured = {};
static gl::Sampler sampler = {};
static gl::VertexArray vao = {};

static void init_program(gl::Program &prog, const vfs::path_t &path, const gl::Shader &vert)
{
    gl::Shader frag = {};

    frag.create(GL_FRAGMENT_SHADER);
    if(!shaders::compile(frag, path))
        std::terminate();

    prog.create();
    prog.attach(vert);
    prog.attach(frag);

    if(!prog.link())
        std::terminate();
    frag.destroy();
}

void ui::init()
{
    gl::Shader vert = {};

    ubo.create();
    ubo.storage(sizeof(UiDraw_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    vert.create(GL_VERTEX_SHADER);
    if(!shaders::compile(vert, "/shaders/ui_screenspace.vert"))
        std::terminate();

    init_program(program_label, "/shaders/ui_label.frag", vert);
    init_program(program_rect_colored, "/shaders/ui_rect_colored.frag", vert);
    init_program(program_rect_textured, "/shaders/ui_rect_textured.frag", vert);

    vert.destroy();

    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    vao.create();
}

void ui::deinit()
{
    vao.destroy();
    sampler.destroy();
    program_rect_textured.destroy();
    program_rect_colored.destroy();
    program_label.destroy();
    ubo.destroy();
}

void ui::draw_rect(int xpos, int ypos, int width, int height, const vector4_t &color)
{
    // FIXME
    int swidth, sheight;
    screen::get_size(swidth, sheight);
    glViewport(0, 0, swidth, sheight);

    UiDraw_UBO uniforms = {};
    uniforms.screen.x = swidth;
    uniforms.screen.y = sheight;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.color = color;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    ubo.write(0, sizeof(uniforms), &uniforms);

    vao.bind();
    program_rect_colored.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ui::draw_rect(int xpos, int ypos, int width, int height, const gl::Texture2D &texture, const vector4_t &color)
{
    // FIXME
    int swidth, sheight;
    screen::get_size(swidth, sheight);
    glViewport(0, 0, swidth, sheight);

    texture.bind(0);
    sampler.bind(0);

    UiDraw_UBO uniforms = {};
    uniforms.screen.x = swidth;
    uniforms.screen.y = sheight;
    uniforms.screen.z = 1.0 / uniforms.screen.x;
    uniforms.screen.w = 1.0 / uniforms.screen.y;
    uniforms.color = color;
    uniforms.rect.x = xpos;
    uniforms.rect.y = ypos;
    uniforms.rect.z = width;
    uniforms.rect.w = height;

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    ubo.write(0, sizeof(uniforms), &uniforms);

    vao.bind();
    program_rect_textured.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ui::draw_label(int xpos, int ypos, const ui::Label &label, const ui::Font *font, const vector2_t &scale, const vector4_t &color)
{
    if(font != nullptr) {
        font->texture.bind(0);
        label.get().bind(1);

        sampler.bind(0);
        sampler.bind(1);

        // FIXME
        int swidth, sheight;
        screen::get_size(swidth, sheight);
        glViewport(0, 0, swidth, sheight);

        UiDraw_UBO uniforms = {};
        uniforms.screen.x = swidth;
        uniforms.screen.y = sheight;
        uniforms.screen.z = 1.0 / uniforms.screen.x;
        uniforms.screen.w = 1.0 / uniforms.screen.y;
        uniforms.glyph.x = scale.x * font->glyph_width;
        uniforms.glyph.y = scale.y * font->glyph_height;
        uniforms.glyph.z = font->texture_cwidth;
        uniforms.glyph.w = font->texture_cheight;
        uniforms.color = color;
        uniforms.rect.x = xpos;
        uniforms.rect.y = ypos;
        uniforms.rect.z = uniforms.glyph.x * label.get_size();
        uniforms.rect.w = uniforms.glyph.y;

        ubo.bind_base(GL_UNIFORM_BUFFER, 0);
        ubo.write(0, sizeof(uniforms), &uniforms);

        vao.bind();
        program_label.bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}


