// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/chunk_mesh.hh>
#include <client/entity/chunk_visible.hh>
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/atlas.hh>
#include <client/camera.hh>
#include <client/chunk_render.hh>
#include <client/globals.hh>
#include <client/quad_vertex.hh>
#include <client/voxel_anims.hh>
#include <glm/gtc/type_ptr.hpp>
#include <entt/entity/registry.hpp>
#include <shared/entity/chunk.hh>
#include <shared/util/coord.hh>
#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h> // FIXME

struct Pipeline final {
    GLuint program {};
    GLint u_camera_matrix {};
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
        spdlog::critical("chunk_render: {}: shader compile failed", name);
        std::terminate();
    }

    pipeline.program = util::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!pipeline.program) {
        spdlog::critical("chunk_render: {}: program link failed", name);
        std::terminate();
    }

    pipeline.u_camera_matrix = glGetUniformLocation(pipeline.program, "u_CameraMatrix");
    pipeline.u_world_position = glGetUniformLocation(pipeline.program, "u_WorldPosition");
    pipeline.u_timings = glGetUniformLocation(pipeline.program, "u_Timings");
}

void chunk_render::init(void)
{
    setup_pipeline(quad_pipeline, "chunk_quad");

    const glm::fvec3 vertices[4] = {
        glm::fvec3(0.0f, 0.0f, -1.0f), glm::fvec3(0.0f, 0.0f, 0.0f),
        glm::fvec3(1.0f, 0.0f, -1.0f), glm::fvec3(1.0f, 0.0f, 0.0f)
    };

    glGenVertexArrays(1, &quad_vaobj);
    glBindVertexArray(quad_vaobj);

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(glm::fvec3), nullptr);

}

void chunk_render::deinit(void)
{
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &quad_vaobj);
    glDeleteProgram(quad_pipeline.program);
}

void chunk_render::render(void)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    if(glfwGetMouseButton(globals::window, GLFW_MOUSE_BUTTON_4)) // FIXME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glm::uvec3 timings = {};
    timings.x = globals::frametime;
    timings.y = globals::frametime_avg;
    timings.z = voxel_anims::frame;

    const glm::fmat4x4 &matrix = camera::matrix();
    const ChunkPos &cam_chunk = camera::chunk_position();

    const auto group = globals::registry.group(entt::get<ChunkComponent, ChunkMeshComponent, ChunkVisibleComponent>);

    for(std::size_t plane_id = 0; plane_id < atlas::plane_count(); ++plane_id) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, atlas::plane_texture(plane_id));
        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(quad_vaobj);
        glUseProgram(quad_pipeline.program);
        glUniformMatrix4fv(quad_pipeline.u_camera_matrix, 1, false, glm::value_ptr(matrix));
        glUniform3uiv(quad_pipeline.u_timings, 1, glm::value_ptr(timings));
        
        for(const auto [entity, chunk, mesh] : group.each()) {
            if(plane_id >= mesh.quad.size())
                continue;
            if(!mesh.quad[plane_id].handle)
                continue;
            if(!mesh.quad[plane_id].size)
                continue;
        
            const glm::fvec3 wpos = util::to_world(chunk.coord - cam_chunk);
            glUniform3fv(quad_pipeline.u_world_position, 1, glm::value_ptr(wpos));

            glBindBuffer(GL_ARRAY_BUFFER, mesh.quad[plane_id].handle);

            glEnableVertexAttribArray(1);
            glVertexAttribDivisor(1, 1);
            glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, sizeof(QuadVertex), nullptr);
            
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mesh.quad[plane_id].size);
        }
    }
}
