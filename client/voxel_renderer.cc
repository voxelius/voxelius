// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/camera.hh>
#include <client/entity/voxel_mesh.hh>
#include <client/globals.hh>
#include <client/glxx/program.hh>
#include <client/glxx/sampler.hh>
#include <client/glxx/vertexarray.hh>
#include <client/shaders.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <client/voxel_renderer.hh>
#include <client/voxel_vertex.hh>
#include <entt/entity/registry.hpp>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <shared/entity/chunk.hh>
#include <shared/world.hh>
#include <spdlog/fmt/fmt.h>

struct VoxelRender_UBO final {
    glm::fmat4x4 viewmat {};
    glm::uvec4 timings {};
    glm::fvec4 chunk {};
};

static glxx::Buffer ubo = {};
static glxx::Sampler sampler = {};
static glxx::VertexArray vao = {};
static glxx::Program program_solid = {};

static void init_program(glxx::Program &prog, const std::string &name)
{
    glxx::Shader vert = {};
    glxx::Shader frag = {};
    std::string source = {};

    vert.create(GL_VERTEX_SHADER);
    frag.create(GL_FRAGMENT_SHADER);

    if(!shaders::compile(vert, fmt::format("/shaders/{}.vert", name)))
        std::terminate();
    if(!shaders::compile(frag, fmt::format("/shaders/{}.frag", name)))
        std::terminate();

    prog.create();
    prog.attach(vert);
    prog.attach(frag);

    if(!prog.link())
        std::terminate();
    vert.destroy();
    frag.destroy();
}

void voxel_renderer::init()
{
    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    ubo.create();
    ubo.storage(sizeof(VoxelRender_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    vao.create();

    VoxelVertex::setup(vao);

    init_program(program_solid, "voxel_solid");
    // init_program(program_cutout, "voxel_cutout");
    // init_program(program_blend, "voxel_blend");
}

void voxel_renderer::deinit()
{
    program_solid.destroy();

    vao.destroy();
    ubo.destroy();

    sampler.destroy();
}

void voxel_renderer::render()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // UNDONE: maintain this via a debug setting
    if(glfwGetMouseButton(globals::window, GLFW_MOUSE_BUTTON_4))
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    vao.bind();

    voxel_atlas::get().bind(0);

    sampler.bind(0);

    VoxelRender_UBO uniforms = {};
    uniforms.viewmat = camera::get_matrix();
    uniforms.timings.x = voxel_anims::frame;
    ubo.bind_base(GL_UNIFORM_BUFFER, 0);

    voxel_anims::bind_ssbo();

    const auto cam_cpos = camera::get_chunk_pos();
    const auto group = globals::registry.group(entt::get<VoxelMeshComponent, ChunkComponent>);

    program_solid.bind();
    for(const auto [entity, mesh, chunk] : group.each()) {
        if(mesh.meshes[VOXEL_DRAW_SOLID].vertices) {
            const auto wcpos = coord::to_world(chunk.cpos - cam_cpos);
            uniforms.chunk = glm::fvec4{wcpos.x, wcpos.y, wcpos.z, 0.0};
            ubo.write(0, sizeof(uniforms), &uniforms);
            vao.set_vertex_buffer(VOXEL_VBO_BINDING, mesh.meshes[VOXEL_DRAW_SOLID].vbo, sizeof(VoxelVertex));
            glDrawArrays(GL_TRIANGLES, 0, mesh.meshes[VOXEL_DRAW_SOLID].vertices);
        }
    }
}
