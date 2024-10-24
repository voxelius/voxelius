// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <game/client/chunk_mesher.hh>
#include <game/client/chunk_quad.hh>
#include <game/client/chunk_renderer.hh>
#include <game/client/chunk_visibility.hh>
#include <game/client/game.hh>
#include <game/client/globals.hh>
#include <game/client/outline.hh>
#include <game/client/skybox.hh>
#include <game/client/toggles.hh>
#include <game/client/varied_program.hh>
#include <game/client/view.hh>
#include <game/client/voxel_anims.hh>
#include <game/client/voxel_atlas.hh>
#include <game/shared/entity/chunk.hh>
#include <spdlog/spdlog.h>

// ONLY TOUCH THESE IF THE RESPECTIVE SHADER
// VARIANT MACRO DECLARATIONS LAYOUT CHANGED AS WELL
constexpr static unsigned int WORLD_CURVATURE = 0U;
constexpr static unsigned int WORLD_FOG = 1U;

static VariedProgram quad_program = {};
static std::size_t u_quad_vproj_matrix = {};
static std::size_t u_quad_world_position = {};
static std::size_t u_quad_timings = {};
static std::size_t u_quad_fog_color = {};
static std::size_t u_quad_view_distance = {};
static std::size_t u_quad_textures = {};
static GLuint quad_vaobj = {};
static GLuint quad_vbo = {};

void chunk_renderer::init(void)
{
    if(!VariedProgram::setup(quad_program, "shaders/chunk_quad.vert", "shaders/chunk_quad.frag")) {
        spdlog::critical("chunk_renderer: quad_program: setup failed");
        std::terminate();
    }

    u_quad_vproj_matrix = VariedProgram::add_uniform(quad_program, "u_ViewProjMatrix");
    u_quad_world_position = VariedProgram::add_uniform(quad_program, "u_WorldPosition");
    u_quad_timings = VariedProgram::add_uniform(quad_program, "u_Timings");
    u_quad_fog_color = VariedProgram::add_uniform(quad_program, "u_FogColor");
    u_quad_view_distance = VariedProgram::add_uniform(quad_program, "u_ViewDistance");
    u_quad_textures = VariedProgram::add_uniform(quad_program, "u_Textures");

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
    VariedProgram::destroy(quad_program);
}

void chunk_renderer::render(void)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(1.0f);

    if(toggles::render_wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    VariedProgram::set_variant_vert(quad_program, WORLD_CURVATURE, client_game::world_curvature);
    VariedProgram::set_variant_vert(quad_program, WORLD_FOG, client_game::fog_mode);
    VariedProgram::set_variant_frag(quad_program, WORLD_FOG, client_game::fog_mode);

    if(!VariedProgram::update(quad_program)) {
        spdlog::critical("chunk_renderer: quad_program: update failed");
        VariedProgram::destroy(quad_program);
        std::terminate();
    }

    GLuint timings[3] = {};
    timings[0] = globals::frametime;
    timings[1] = globals::frametime_avg;
    timings[2] = voxel_anims::frame;

    const auto group = globals::registry.group(entt::get<ChunkComponent, ChunkMeshComponent, ChunkVisibleComponent>);

    for(std::size_t plane_id = 0; plane_id < voxel_atlas::plane_count(); ++plane_id) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, voxel_atlas::plane_texture(plane_id));

        glBindVertexArray(quad_vaobj);

        glUseProgram(quad_program.handle);
        glUniformMatrix4fv(quad_program.uniforms[u_quad_vproj_matrix].location, 1, false, view::matrix.data()->data());
        glUniform3uiv(quad_program.uniforms[u_quad_timings].location, 1, timings);
        glUniform4fv(quad_program.uniforms[u_quad_fog_color].location, 1, skybox::fog_color.data());
        glUniform1f(quad_program.uniforms[u_quad_view_distance].location, view::max_distance * CHUNK_SIZE);
        glUniform1i(quad_program.uniforms[u_quad_textures].location, 0); // GL_TEXTURE0

        for(const auto [entity, chunk, mesh] : group.each()) {
            if(plane_id >= mesh.quad.size())
                continue;
            if(!mesh.quad[plane_id].handle)
                continue;
            if(!mesh.quad[plane_id].size)
                continue;

            const Vec3f wpos = ChunkCoord::to_vec3f(chunk.coord - view::position.chunk);
            glUniform3fv(quad_program.uniforms[u_quad_world_position].location, 1, wpos.data());

            glBindBuffer(GL_ARRAY_BUFFER, mesh.quad[plane_id].handle);

            glEnableVertexAttribArray(1);
            glVertexAttribDivisor(1, 1);
            glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, sizeof(ChunkQuad), nullptr);
            
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mesh.quad[plane_id].size);
            
            globals::num_drawcalls += 1;
            globals::num_triangles += 2 * mesh.quad[plane_id].size;
        }
    }

    if(toggles::draw_chunk_borders) {
        outline::prepare();

        for(const auto [entity, chunk, mesh] : group.each()) {
            const WorldCoord wpos = ChunkCoord::to_world(chunk.coord);
            const Vec3f size = Vec3f(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
            outline::cube(wpos, size, 1.0f, Vec4f::yellow());
        }
    }
}
