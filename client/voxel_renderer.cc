// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/glxx/program.hh>
#include <client/glxx/sampler.hh>
#include <client/glxx/vertex_array.hh>
#include <client/screen.hh>
#include <client/shaders.hh>
#include <client/view.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
#include <client/voxel_vertex.hh>
#include <shared/types.hh>
#include <shared/vfs.hh>
#include <shared/world.hh>
#include <spdlog/fmt/fmt.h>

struct VoxelRender_UBO final {
    matrix4x4f_t viewmat {};
    vector4u_t timings {};
    vector4f_t chunk {};
};

static glxx::Buffer ubo = {};
static glxx::Sampler sampler = {};
static glxx::VertexArray vao = {};
static glxx::Program program_solid = {};

static std::vector<entt::entity> chunks_solid = {};
static std::vector<entt::entity> chunks_cutout = {};
static std::vector<entt::entity> chunks_blend = {};

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

    init_program(program_solid, "gbuffer_solid");
    // init_program(program_cutout, "gbuffer_cutout");
    // init_program(program_blend, "gbuffer_blend");
}

void voxel_renderer::deinit()
{
    chunks_blend.clear();
    chunks_cutout.clear();
    chunks_solid.clear();

    program_solid.destroy();

    vao.destroy();
    ubo.destroy();

    sampler.destroy();
}

void voxel_renderer::render()
{
    VoxelRender_UBO uniforms = {};

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // UNDONE: maintain this via a debug setting
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepthf(1.0f);

    vao.bind();

    atlas::get().bind(0);

    sampler.bind(0);

    uniforms.viewmat = view::get_matrix();
    uniforms.timings.x = voxel_anims::frame;
    ubo.bind_base(GL_UNIFORM_BUFFER, 1);

    voxel_anims::bind_ssbo();

    int width, height;
    screen::get_size(width, height);
    glViewport(0, 0, width, height);

    chunks_solid.clear();
    chunks_cutout.clear();
    chunks_blend.clear();

    const auto group = globals::world.registry.group(entt::get<VoxelMeshComponent, ChunkComponent>);
    for(const auto [entity, mesh, chunk] : group.each()) {
        if(mesh.meshes[VOXEL_DRAW_SOLID].vertices)
            chunks_solid.push_back(entity);
        if(mesh.meshes[VOXEL_DRAW_CUTOUT].vertices)
            chunks_cutout.push_back(entity);
        if(mesh.meshes[VOXEL_DRAW_BLEND].vertices)
            chunks_blend.push_back(entity);
    }

    program_solid.bind();
    globals::gbuffer_solid.get_framebuffer().bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for(const auto entity : chunks_solid) {
        const auto &chunk = globals::world.registry.get<ChunkComponent>(entity);
        const auto &mesh = globals::world.registry.get<VoxelMeshComponent>(entity);
        const auto &mref = mesh.meshes[VOXEL_DRAW_SOLID];

        const auto wcpos = coord::to_world(chunk.cpos);
        uniforms.chunk = vector4f_t{wcpos.x, wcpos.y, wcpos.z, 0.0f};
        ubo.write(0, sizeof(uniforms), &uniforms);

        vao.set_vertex_buffer(VOXEL_VERTEX_VBO_BINDING, mref.vbo, sizeof(VoxelVertex));

        glDrawArrays(GL_TRIANGLES, 0, mref.vertices);
    }
}
