// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/postprocess.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/glxx/program.hh>
#include <client/glxx/sampler.hh>
#include <client/glxx/vertex_array.hh>
#include <client/screen.hh>
#include <client/shaders.hh>
#include <shared/vfs.hh>

static glxx::Sampler sampler = {};
static glxx::VertexArray vao = {};
static glxx::Program program = {};

void postprocess::init()
{
    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    vao.create();

    glxx::Shader vert = {};
    glxx::Shader frag = {};
    std::string source = {};

    vert.create(GL_VERTEX_SHADER);
    frag.create(GL_FRAGMENT_SHADER);

    if(!shaders::compile(vert, "/shaders/postprocess.vert"))
        std::terminate();
    if(!shaders::compile(frag, "/shaders/postprocess.frag"))
        std::terminate();

    program.create();
    program.attach(vert);
    program.attach(frag);

    if(!program.link())
        std::terminate();
    vert.destroy();
    frag.destroy();
}

void postprocess::deinit()
{
    program.destroy();

    vao.destroy();

    sampler.destroy();
}

void postprocess::render()
{
    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glxx::Framebuffer::unbind();
    glClear(GL_COLOR_BUFFER_BIT);

    globals::deferred_color.bind(0);

    sampler.bind(0);

    int width, height;
    screen::get_size(width, height);
    glViewport(0, 0, width, height);

    vao.bind();

    program.bind();

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
