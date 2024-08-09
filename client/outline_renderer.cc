// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/outline_renderer.hh>
#include <client/varied_program.hh>
#include <client/view.hh>
#include <spdlog/spdlog.h>

// ONLY TOUCH THESE IF THE RESPECTIVE SHADER
// VARIANT MACRO DECLARATIONS LAYOUT CHANGED AS WELL
constexpr static unsigned int WORLD_CURVATURE = 0U;

static VariedProgram program = {};
static std::size_t u_world_position = {};
static std::size_t u_vproj_matrix = {};
static std::size_t u_scale = {};
static std::size_t u_color = {};

static GLuint vaobj = {};
static GLuint cube_vbo = {};
static GLuint line_vbo = {};

void outline_renderer::init(void)
{
    // A single program is used. Internally it keeps
    // an array of transformations depending on what
    // instance is being rendered currently. At most
    // the progam can be called for six instances with
    // instance zero being also used for line drawing.
    // It is obscure but it allows me to use less OpenGL
    if(!VariedProgram::setup(program, "shaders/outline.vert", "shaders/outline.frag")) {
        spdlog::critical("outline_renderer: program setup failed");
        std::terminate();
    }
    
    u_vproj_matrix = VariedProgram::uniform(program, "u_ViewProjMatrix");
    u_world_position = VariedProgram::uniform(program, "u_WorldPosition");
    u_scale = VariedProgram::uniform(program, "u_Scale");
    u_color = VariedProgram::uniform(program, "u_Color");

    const Vec3f cube_vertices[4] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
    };

    const Vec3f line_vertices[2] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 1.0f),
    };

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);

}

void outline_renderer::deinit(void)
{
    glDeleteBuffers(1, &line_vbo);
    glDeleteBuffers(1, &cube_vbo);
    glDeleteVertexArrays(1, &vaobj);
    VariedProgram::destroy(program);
}

void outline_renderer::begin(void)
{
    VariedProgram::variant_vert(program, WORLD_CURVATURE, 0);

    if(!VariedProgram::update(program)) {
        spdlog::critical("outline_renderer: program update failed");
        VariedProgram::destroy(program);
        std::terminate();
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUseProgram(program.handle);
    glUniformMatrix4fv(program.uniforms[u_vproj_matrix].location, 1, false, view::matrix.data()->data());

    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);

}

void outline_renderer::begin_depth(void)
{
    VariedProgram::variant_vert(program, WORLD_CURVATURE, 0);

    if(!VariedProgram::update(program)) {
        spdlog::critical("outline_renderer: program update failed");
        VariedProgram::destroy(program);
        std::terminate();
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUseProgram(program.handle);
    glUniformMatrix4fv(program.uniforms[u_vproj_matrix].location, 1, false, view::matrix.data()->data());

    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);
}

void outline_renderer::cube(const WorldCoord &start, const Vec3f &scale, float width, const Vec4f &color)
{
    WorldCoord patch = start;
    patch.chunk -= view::position.chunk;

    glLineWidth(width);
    
    glUniform3fv(program.uniforms[u_world_position].location, 1, WorldCoord::to_vec3f(patch).data());
    glUniform3fv(program.uniforms[u_scale].location, 1, scale.data());
    glUniform4fv(program.uniforms[u_color].location, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);
    glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 6);
}

void outline_renderer::line(const WorldCoord &start, const Vec3f &scale, float width, const Vec4f &color)
{
    WorldCoord patch = start;
    patch.chunk -= view::position.chunk;

    glLineWidth(width);
    
    glUniform3fv(u_world_position, 1, WorldCoord::to_vec3f(patch).data());
    glUniform3fv(u_scale, 1, scale.data());
    glUniform4fv(u_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);
    glDrawArrays(GL_LINES, 0, 2);
}

