// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/postprocess.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/gl/program.hh>
#include <client/gl/sampler.hh>
#include <client/gl/vertexarray.hh>
#include <client/shaders.hh>
#include <shared/vfs.hh>

static gl::Sampler sampler = {};
static gl::VertexArray vao = {};
static gl::Program program = {};

void postprocess::init()
{
    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    vao.create();

    gl::Shader vert = {};
    gl::Shader frag = {};
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
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    globals::deferred_color.bind(0);

    sampler.bind(0);

    glViewport(0, 0, globals::window_width, globals::window_height);

    vao.bind();
    program.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
