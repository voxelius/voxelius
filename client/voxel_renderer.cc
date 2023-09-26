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
#include <client/view.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
#include <shared/types.hh>
#include <shared/vfs.hh>
#include <shared/world.hh>
#include <spdlog/fmt/fmt.h>

struct VoxelRender_UBO final {
    mat4x4f_t vpmat {};
    vec4f_t chunk {};
};

static glxx::Buffer ubo = {};
static glxx::Sampler sampler = {};
static glxx::Program prog_opaque = {};
static glxx::VertexArray vao = {};

static void init_program(glxx::Program &prog, const std::string &name)
{
    glxx::Shader vert = {};
    glxx::Shader frag = {};
    std::string source = {};

    vert.create(GL_VERTEX_SHADER);
    frag.create(GL_FRAGMENT_SHADER);

    if(!vfs::read_string(fmt::format("/shaders/{}.vert", name), source) || !vert.glsl(source))
        std::terminate();
    if(!vfs::read_string(fmt::format("/shaders/{}.frag", name), source) || !frag.glsl(source))
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
    ubo.create();
    ubo.storage(sizeof(VoxelRender_UBO), nullptr, GL_DYNAMIC_STORAGE_BIT);

    sampler.create();
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    init_program(prog_opaque, "gbuffer_opaque");
    // init_program(prog_alpha, "gbuffer_alpha");
    // init_program(prog_fluid, "gbuffer_fluid");

    vao.create();

    // Attachment #0: FLOAT3, position
    vao.enable_attribute(0, true);
    vao.set_attribute_format(0, GL_FLOAT, 3, offsetof(VoxelVertex, position), false);
    vao.set_attribute_binding(0, 0);

    // Attachment #1: FLOAT3, normal
    vao.enable_attribute(1, true);
    vao.set_attribute_format(1, GL_FLOAT, 3, offsetof(VoxelVertex, norm), false);
    vao.set_attribute_binding(1, 0);

    // Attachment #2: FLOAT2, UV coords
    vao.enable_attribute(2, true);
    vao.set_attribute_format(2, GL_FLOAT, 2, offsetof(VoxelVertex, tex_uv), false);
    vao.set_attribute_binding(2, 0);

    // Attachment #3: UINT1, atlas index
    vao.enable_attribute(3, true);
    vao.set_attribute_format(3, GL_UNSIGNED_INT, 1, offsetof(VoxelVertex, tex_id), false);
    vao.set_attribute_binding(3, 0);
}

void voxel_renderer::deinit()
{
    vao.destroy();
    prog_opaque.destroy();
    sampler.destroy();
    ubo.destroy();
}

void voxel_renderer::render()
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // UNDONE: maintain this via a debug setting
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepthf(1.0f);

    vao.bind();

    atlas::get().bind(0);

    sampler.bind(0);

    VoxelRender_UBO uniforms = {};
    uniforms.vpmat = view::get_matrix();
    ubo.bind_base(GL_UNIFORM_BUFFER, 1);

    int width, height;
    screen::get_size(width, height);
    glViewport(0, 0, width, height);

    const auto group = globals::world.registry.group(entt::get<VoxelMeshComponent, ChunkComponent>);

    // Crutch!
    globals::gbuffer_opaque.get_framebuffer().bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(const auto [entity, mc, chunk] : group.each()) {
        uniforms.chunk = vec4f_t{coord::to_world(chunk.cpos), 0.0f};
        ubo.write(0, sizeof(uniforms), &uniforms);

        // Crutch!
        const Mesh &mref = mc.meshes[VOXEL_DRAW_OPAQUE];

        // Crutch!
        prog_opaque.bind();

        // Crutch!
        globals::gbuffer_opaque.get_framebuffer().bind();

        vao.set_vertex_buffer(0, mref.vbo, sizeof(VoxelVertex));

        glDrawArrays(GL_TRIANGLES, 0, mref.vertices);
    }
}
