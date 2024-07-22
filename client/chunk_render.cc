// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/chunk_mesh.hh>
#include <client/entity/chunk_visible.hh>
#include <client/util/program.hh>
#include <client/util/shader.hh>
#include <client/camera.hh>
#include <client/chunk_render.hh>
#include <client/globals.hh>
#include <client/quad_vertex.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <glm/gtc/type_ptr.hpp>
#include <entt/entity/registry.hpp>
#include <shared/entity/chunk.hh>
#include <shared/util/coord.hh>
#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h> // FIXME

constexpr static GLuint UBO_ANIMATIONS_BINDING = 0;

struct ChunkProgram final {
    GLuint gl_program {};
    GLint u_camera_matrix {};
    GLint u_world_position {};
    GLint u_timings {};
};

static ChunkProgram quad_animated = {};
static ChunkProgram quad_varied = {};
static GLuint quad_vaobj = {};
static GLuint quad_vbo = {};

static void setup_program(ChunkProgram &program, const std::string &name)
{
    const std::string vert_path = fmt::format("/shaders/{}.vert", name);
    const std::string frag_path = fmt::format("/shaders/{}.frag", name);

    GLuint vert = util::compile_shader(vert_path, GL_VERTEX_SHADER);
    GLuint frag = util::compile_shader(frag_path, GL_FRAGMENT_SHADER);

    if(!vert || !frag) {
        spdlog::critical("chunk_render: {}: shader compile failed", name);
        std::terminate();
    }

    program.gl_program = util::link_program(vert, frag);

    glDeleteShader(frag);
    glDeleteShader(vert);

    if(!program.gl_program) {
        spdlog::critical("chunk_render: {}: program link failed", name);
        std::terminate();
    }

    program.u_camera_matrix = glGetUniformLocation(program.gl_program, "u_CameraMatrix");
    program.u_world_position = glGetUniformLocation(program.gl_program, "u_WorldPosition");
    program.u_timings = glGetUniformLocation(program.gl_program, "u_Timings");

    const GLuint animations_index = glGetUniformBlockIndex(program.gl_program, "u_Animations");

    if(animations_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(program.gl_program, animations_index, UBO_ANIMATIONS_BINDING);
    }
}

static void draw_quads(std::size_t plane_id, const ChunkProgram &program, const ChunkPos &cam_cpos, bool blending)
{
    const auto group = globals::registry.group(entt::get<ChunkComponent, ChunkMeshComponent, ChunkVisibleComponent>);

    for(const auto [entity, chunk, mesh] : group.each()) {
        if(mesh.quad.size() <= plane_id)
            continue;
        if(!mesh.quad[plane_id].handle)
            continue;
        if(!mesh.quad[plane_id].size)
            continue;

        const glm::fvec3 wpos = util::to_world(chunk.coord - cam_cpos);
        glUniform3fv(program.u_world_position, 1, glm::value_ptr(wpos));

        glBindBuffer(GL_ARRAY_BUFFER, mesh.quad[plane_id].handle);

        glEnableVertexAttribArray(1);
        glVertexAttribDivisor(1, 1);
        glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, sizeof(QuadVertex), nullptr);
        
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, mesh.quad[plane_id].size);
    }
}

void chunk_render::init(void)
{
    setup_program(quad_animated, "chunk_quad_animated");
    setup_program(quad_varied, "chunk_quad_varied");

    const glm::fvec3 vertices[6] = {
        glm::fvec3(0.0f, 0.0f,  0.0f),
        glm::fvec3(0.0f, 0.0f, -1.0f),
        glm::fvec3(1.0f, 0.0f, -1.0f),

        glm::fvec3(1.0f, 0.0f, -1.0f),
        glm::fvec3(1.0f, 0.0f,  0.0f),
        glm::fvec3(0.0f, 0.0f,  0.0f),
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
    glDeleteProgram(quad_varied.gl_program);
    glDeleteProgram(quad_animated.gl_program);
}

void chunk_render::render(void)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    if(glfwGetMouseButton(globals::window, GLFW_MOUSE_BUTTON_4)) // FIXME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_ANIMATIONS_BINDING, voxel_anims::buffer());

    glm::uvec3 timings = {};
    timings.x = globals::frametime;
    timings.y = globals::frametime_avg;
    timings.z = voxel_anims::frame;

    const glm::fmat4x4 &matrix = camera::matrix();
    const ChunkPos &cam_chunk = camera::chunk_position();

    for(std::size_t plane = 0; plane < voxel_atlas::plane_count(); ++plane) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, voxel_atlas::plane_texture(plane));
        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(quad_vaobj);

        if(plane == ANIMATION_PLANE_ID) {
            glUseProgram(quad_animated.gl_program);
            glUniformMatrix4fv(quad_animated.u_camera_matrix, 1, false, glm::value_ptr(matrix));
            glUniform3uiv(quad_animated.u_timings, 1, glm::value_ptr(timings));
            draw_quads(plane, quad_animated, cam_chunk, false);
        }
        else {
            glUseProgram(quad_varied.gl_program);
            glUniformMatrix4fv(quad_varied.u_camera_matrix, 1, false, glm::value_ptr(matrix));
            glUniform3uiv(quad_varied.u_timings, 1, glm::value_ptr(timings));
            draw_quads(plane, quad_varied, cam_chunk, false);
        }
    }
}
