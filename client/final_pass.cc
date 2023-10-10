// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/final_pass.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/glxx/program.hh>
#include <client/glxx/sampler.hh>
#include <client/glxx/vertexarray.hh>
#include <client/shaders.hh>
#include <shared/vfs.hh>

static glxx::Sampler sampler = {};
static glxx::VertexArray vao = {};
static glxx::Program program = {};

void final_pass::init()
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

    if(!shaders::compile(vert, "/shaders/final_pass.vert"))
        std::terminate();
    if(!shaders::compile(frag, "/shaders/final_pass.frag"))
        std::terminate();

    program.create();
    program.attach(vert);
    program.attach(frag);

    if(!program.link())
        std::terminate();
    vert.destroy();
    frag.destroy();
}

void final_pass::deinit()
{
    program.destroy();

    vao.destroy();

    sampler.destroy();
}

void final_pass::render()
{
    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    globals::deferred_color.bind(0);

    sampler.bind(0);

    glViewport(0, 0, globals::window_width, globals::window_height);

    vao.bind();
    program.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
