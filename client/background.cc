// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <client/background.hh>
#include <client/globals.hh>
#include <client/glxx/program.hh>
#include <client/glxx/vertexarray.hh>
#include <client/shaders.hh>
#include <GLFW/glfw3.h>
#include <glm/vec4.hpp>
#include <glm/fwd.hpp>
#include <shared/const.hh>

struct Background_UBO final {
    glm::fvec4 time {};
};

static glxx::Buffer ubo = {};
static glxx::Program program = {};
static glxx::VertexArray vao = {};

void background::init()
{
    glxx::Shader vert = {};
    glxx::Shader frag = {};

    ubo.create();
    ubo.storage(sizeof(Background_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    vert.create(GL_VERTEX_SHADER);
    frag.create(GL_FRAGMENT_SHADER);

    if(!shaders::compile(vert, "/shaders/background.vert"))
        std::terminate();
    if(!shaders::compile(frag, "/shaders/background.frag"))
        std::terminate();

    program.create();
    program.attach(vert);
    program.attach(frag);

    if(!program.link())
        std::terminate();
    frag.destroy();
    vert.destroy();

    vao.create();
}

void background::deinit()
{
    vao.destroy();
    program.destroy();
    ubo.destroy();
}

void background::render()
{
    Background_UBO uniforms = {};
    uniforms.time.x = glfwGetTime();

    ubo.bind_base(GL_UNIFORM_BUFFER, 0);
    ubo.write(0, sizeof(uniforms), &uniforms);

    program.bind();

    vao.bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
