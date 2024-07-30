// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug/debug_toggles.hh>
#include <client/util/shader.hh>
#include <client/util/program.hh>
#include <client/world/chunk_mesh.hh>
#include <client/world/chunk_renderer.hh>
#include <client/world/chunk_visibility.hh>
#include <client/world/quad_vertex.hh>
#include <client/world/vertex_buffer.hh>
#include <client/world/voxel_anims.hh>
#include <client/world/voxel_atlas.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <shared/world/chunk.hh>
#include <spdlog/spdlog.h>

struct Pipeline final {
    GLuint program {};
    GLint u_vproj_matrix {};
    GLint u_world_position {};
    GLint u_timings {};
};

static Pipeline quad_pipeline = {};
static GLuint quad_vaobj = {};
static GLuint quad_vbo = {};

static void setup_pipeline(Pipeline &pipeline, const std::string &name)
{
    const std::string vert_path = fmt::format("shaders/{}.vert", name);
    const std::string frag_path = fmt::format("shaders/{}.frag", name);

    GLuint vert = util::compile_shader(vert_path, GL_VERTEX_SHADER);
    GLuint frag = util::compile_shader(frag_path, GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        spdlog::critical("chunk_renderer: {}: shader compile failed", name);
        std::terminate();
    }

    pipeline.program = util::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!pipeline.program) {
        spdlog::critical("chunk_renderer: {}: program link failed", name);
        std::terminate();
    }

    pipeline.u_vproj_matrix = glGetUniformLocation(pipeline.program, "u_ViewProjMatrix");
    pipeline.u_world_position = glGetUniformLocation(pipeline.program, "u_WorldPosition");
    pipeline.u_timings = glGetUniformLocation(pipeline.program, "u_Timings");
}

void chunk_renderer::init(void)
{
    setup_pipeline(quad_pipeline, "chunk_quad");

    const Vec3f vertices[4] = {
        Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(0.0f, 0.0f, 0.0f),
    };

    glGenVertexArrays(1, &quad_vaobj);
    glBindVertexArray(quad_vaobj);

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);

}

void chunk_renderer::deinit(void)
{
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &quad_vaobj);
    glDeleteProgram(quad_pipeline.program);
}

void chunk_renderer::render(void)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(1.0f);

    if(debug_toggles::render_wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLuint timings[3] = {};
    timings[0] = globals::frametime;
    timings[1] = globals::frametime_avg;
    timings[2] = voxel_anims::frame;

    const auto group = globals::registry.group(entt::get<ChunkComponent, ChunkMeshComponent, ChunkVisibleComponent>);

    for(std::size_t plane_id = 0; plane_id < voxel_atlas::plane_count(); ++plane_id) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, voxel_atlas::plane_texture(plane_id));
        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(quad_vaobj);
        glUseProgram(quad_pipeline.program);
        glUniformMatrix4fv(quad_pipeline.u_vproj_matrix, 1, false, view::matrix.data()->data());
        glUniform3uiv(quad_pipeline.u_timings, 1, timings);
        
        for(const auto [entity, chunk, mesh] : group.each()) {
            if(plane_id >= mesh.quad.size())
                continue;
            if(!mesh.quad[plane_id].handle)
                continue;
            if(!mesh.quad[plane_id].size)
                continue;

            const Vec3f wpos = ChunkCoord::to_vec3f(chunk.coord - view::position.chunk);
            glUniform3fv(quad_pipeline.u_world_position, 1, wpos.data());

            glBindBuffer(GL_ARRAY_BUFFER, mesh.quad[plane_id].handle);

            glEnableVertexAttribArray(1);
            glVertexAttribDivisor(1, 1);
            glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, sizeof(QuadVertex), nullptr);
            
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mesh.quad[plane_id].size);
            
            globals::num_triangles += 2 * mesh.quad[plane_id].size;
        }
    }
}
