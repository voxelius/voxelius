// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <common/fstools.hh>
#include <common/image.hh>
#include <game/client/background.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/settings.hh>
#include <game/client/varied_program.hh>
#include <mathlib/constexpr.hh>
#include <mathlib/mat4x4f.hh>
#include <mathlib/vec2f.hh>
#include <random>
#include <spdlog/spdlog.h>

constexpr static int RT_SIZE = 512;

static bool enable_3D = false;

static Mat4x4f mdmatrix = {};

static int texture_width = {};
static int texture_height = {};
static GLuint texture = {};

static VariedProgram program_2D = {};
static std::size_t u_texture = {};
static std::size_t u_scale = {};

static VariedProgram program_3D = {};
static std::size_t u_vpmatrix = {};
static std::size_t u_mdmatrix = {};

static VariedProgram program_post = {};
static std::size_t u_rt_texture = {};
static GLuint rt_framebuffer = {};
static GLuint rt_texture = {};

static GLuint cube_vaobj = {};
static GLuint cube_vbo = {};

static GLuint quad_vaobj = {};
static GLuint quad_vbo = {};

void background::init(void)
{
    Config::add(globals::client_config, "background.enable_3D", enable_3D);

    settings::add_checkbox(0, settings::VIDEO_GUI, "background.enable_3D", enable_3D, true);


    std::random_device random = {};
    std::uniform_real_distribution<float> dist = std::uniform_real_distribution<float>(0.0f, 90.0f);

    mdmatrix = Mat4x4f::identity();
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(1.0f, 0.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(0.0f, 1.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(0.0f, 0.0f, 1.0f));


    const std::string path = std::string("textures/gui/background.png");

    Image image = {};

    if(!Image::load_rgba(image, path, true)) {
        //spdlog::critical("background: {}: {}", path, fstools::error());
        std::terminate();
    }

    texture_width = image.width;
    texture_height = image.height;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    Image::unload(image);


    if(!VariedProgram::setup(program_2D, "shaders/background_2D.vert", "shaders/background_2D.frag")) {
        spdlog::critical("background: 2D program setup failed");
        std::terminate();
    }

    if(!VariedProgram::setup(program_3D, "shaders/background_3D.vert", "shaders/background_3D.frag")) {
        spdlog::critical("background: 3D program setup failed");
        std::terminate();
    }

    if(!VariedProgram::setup(program_post, "shaders/background_post.vert", "shaders/background_post.frag")) {
        spdlog::critical("background: post-process program setup failed");
        std::terminate();
    }

    u_texture = VariedProgram::add_uniform(program_2D, "u_Texture");
    u_scale = VariedProgram::add_uniform(program_2D, "u_Scale");
 
    u_vpmatrix = VariedProgram::add_uniform(program_3D, "u_ViewProjMatrix");
    u_mdmatrix = VariedProgram::add_uniform(program_3D, "u_ModelMatrix");

    u_rt_texture = VariedProgram::add_uniform(program_post, "u_RT_Texture");

    glGenTextures(1, &rt_texture);
    glBindTexture(GL_TEXTURE_2D, rt_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, RT_SIZE, RT_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glGenFramebuffers(1, &rt_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rt_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt_texture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::critical("background: rt_framebuffer is incomplete");
        std::terminate();
    }

    const Vec3f cube_vertices[4] = {
        Vec3f(0.0f, 0.0f, 0.0f),
        Vec3f(0.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 1.0f),
        Vec3f(1.0f, 0.0f, 0.0f),
    };

    glGenVertexArrays(1, &cube_vaobj);
    glBindVertexArray(cube_vaobj);

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);

    const Vec2f quad_vertices[4] = {
        Vec2f(0.0f, 1.0f),
        Vec2f(0.0f, 0.0f),
        Vec2f(1.0f, 1.0f),
        Vec2f(1.0f, 0.0f),
    };

    glGenVertexArrays(1, &quad_vaobj);
    glBindVertexArray(quad_vaobj);

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vec2f), nullptr);
}

void background::deinit(void)
{
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &quad_vaobj);

    glDeleteBuffers(1, &cube_vbo);
    glDeleteVertexArrays(1, &cube_vaobj);

    glDeleteTextures(1, &rt_texture);
    glDeleteFramebuffers(1, &rt_framebuffer);

    VariedProgram::destroy(program_post);
    VariedProgram::destroy(program_3D);
    VariedProgram::destroy(program_2D);

    glDeleteTextures(1, &texture);
}


#include <imgui.h>
void background::render(void)
{
    if(!VariedProgram::update(program_2D)) {
        spdlog::critical("background: 2D program update failed");
        std::terminate();
    }

    if(!VariedProgram::update(program_3D)) {
        spdlog::critical("background: 3D program update failed");
        std::terminate();
    }

    if(!VariedProgram::update(program_post)) {
        spdlog::critical("background: post-process program update failed");
        std::terminate();
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const float angle = 5.0f * cxpr::radians(globals::frametime);
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, 1.00f * angle, Vec3f(1.0f, 0.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, 0.50f * angle, Vec3f(0.0f, 1.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, 0.25f * angle, Vec3f(0.0f, 0.0f, 1.0f));

    if(enable_3D && (globals::gui_screen == GUI_MAIN_MENU)) {
        const Mat4x4f vpmatrix = Mat4x4f::proj_ortho(-3.0f * globals::aspect, 3.0f * globals::aspect, -3.0f, 3.0f, -8.0f, 8.0f);

        glViewport(0, 0, RT_SIZE, RT_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, rt_framebuffer);
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program_3D.handle);
        glUniformMatrix4fv(program_3D.uniforms[u_vpmatrix].location, 1, false, vpmatrix.data()->data());
        glUniformMatrix4fv(program_3D.uniforms[u_mdmatrix].location, 1, false, mdmatrix.data()->data());

        glLineWidth(4.0f);
        glBindVertexArray(cube_vaobj);
        glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 6);
        
        glViewport(0, 0, globals::width, globals::height);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        ImTextureID t = reinterpret_cast<ImTextureID>(static_cast<std::uintptr_t>(rt_texture));
        ImGui::Image(t, ImVec2(RT_SIZE, RT_SIZE));

        glBindTexture(GL_TEXTURE_2D, rt_texture);
        glActiveTexture(GL_TEXTURE0);

        glUseProgram(program_post.handle);
        glUniform1i(program_post.uniforms[u_rt_texture].location, 0); // GL_TEXTURE0

        glBindVertexArray(quad_vaobj);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    else {
        const float scaled_width = 0.75f * static_cast<float>(globals::width / globals::gui_scale);
        const float scaled_height = 0.75f * static_cast<float>(globals::height / globals::gui_scale);
        const float scale_x = scaled_width / static_cast<float>(texture_width);
        const float scale_y = scaled_height / static_cast<float>(texture_height);

        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE0);

        glUseProgram(program_2D.handle);
        glUniform2f(program_2D.uniforms[u_scale].location, scale_x, scale_y);
        glUniform1i(program_2D.uniforms[u_texture].location, 0); // GL_TEXTURE0

        glBindVertexArray(quad_vaobj);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}
