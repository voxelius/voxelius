// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <common/fstools.hh>
#include <common/image.hh>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_framebuffer_size.hh>
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

static bool enable_3D = true;

static VariedProgram cube_program = {};
static VariedProgram tile_program = {};
static std::size_t u_vpmatrix = {};
static std::size_t u_mdmatrix = {};
static std::size_t u_rtexture = {};
static std::size_t u_texture = {};
static std::size_t u_scale = {};

static int texture_width = {};
static int texture_height = {};
static GLuint texture = {};

static int rtexture_width = {};
static int rtexture_height = {};
static GLuint rtexture_fbo = {};
static GLuint rtexture = {};

static GLuint vaobj = {};
static GLuint cube_vbo = {};
static GLuint tile_vbo = {};

static Mat4x4f mdmatrix = {};

static void on_glfw_framebuffer_size(const GlfwFramebufferSizeEvent &event)
{
    rtexture_width = event.width / globals::gui_scale;
    rtexture_height = event.height / globals::gui_scale;

    glGenTextures(1, &rtexture);
    glBindTexture(GL_TEXTURE_2D, rtexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, rtexture_width, rtexture_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &rtexture_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rtexture_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rtexture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::critical("background: rtexture framebuffer is incomplete");
        std::terminate();
    }
}

void background::init(void)
{
    Config::add(globals::client_config, "background.enable_3D", enable_3D);

    settings::add_checkbox(0, settings::VIDEO_GUI, "background.enable_3D", enable_3D, true);

    auto random = std::random_device();
    auto dist = std::uniform_real_distribution<float>(0.0f, 90.0f);

    mdmatrix = Mat4x4f::identity();
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(1.0f, 0.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(0.0f, 1.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(0.0f, 0.0f, 1.0f));

    if(!VariedProgram::setup(cube_program, "shaders/gui_cube.vert", "shaders/gui_cube.frag")) {
        spdlog::critical("background: cube program setup failed");
        std::terminate();
    }

    u_vpmatrix = VariedProgram::add_uniform(cube_program, "u_ViewProjMatrix");
    u_mdmatrix = VariedProgram::add_uniform(cube_program, "u_ModelMatrix");

    if(!VariedProgram::setup(tile_program, "shaders/gui_tile.vert", "shaders/gui_tile.frag")) {
        spdlog::critical("background: tile program setup failed");
        std::terminate();
    }

    u_scale = VariedProgram::add_uniform(tile_program, "u_Scale");
    u_texture = VariedProgram::add_uniform(tile_program, "u_Texture");
    u_rtexture = VariedProgram::add_uniform(tile_program, "u_RTexture");

    const Vec3f cube_vertices[24] = {
        Vec3f(-1.0f, -1.0f, -1.0f), Vec3f(-1.0f, +1.0f, -1.0f),
        Vec3f(-1.0f, +1.0f, -1.0f), Vec3f(+1.0f, +1.0f, -1.0f),
        Vec3f(+1.0f, +1.0f, -1.0f), Vec3f(+1.0f, -1.0f, -1.0f),
        Vec3f(+1.0f, -1.0f, -1.0f), Vec3f(-1.0f, -1.0f, -1.0f),

        Vec3f(-1.0f, -1.0f, +1.0f), Vec3f(-1.0f, +1.0f, +1.0f),
        Vec3f(-1.0f, +1.0f, +1.0f), Vec3f(+1.0f, +1.0f, +1.0f),
        Vec3f(+1.0f, +1.0f, +1.0f), Vec3f(+1.0f, -1.0f, +1.0f),
        Vec3f(+1.0f, -1.0f, +1.0f), Vec3f(-1.0f, -1.0f, +1.0f),

        Vec3f(-1.0f, -1.0f, -1.0f), Vec3f(-1.0f, -1.0f, +1.0f),
        Vec3f(-1.0f, +1.0f, -1.0f), Vec3f(-1.0f, +1.0f, +1.0f),
        Vec3f(+1.0f, -1.0f, -1.0f), Vec3f(+1.0f, -1.0f, +1.0f),
        Vec3f(+1.0f, +1.0f, -1.0f), Vec3f(+1.0f, +1.0f, +1.0f),
    };

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    const Vec2f tile_vertices[4] = {
        Vec2f(0.0f, 1.0f),
        Vec2f(0.0f, 0.0f),
        Vec2f(1.0f, 1.0f),
        Vec2f(1.0f, 0.0f),
    };

    glGenBuffers(1, &tile_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tile_vertices), tile_vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);

    Image image = {};
    if(!Image::load_rgba(image, std::string("textures/gui/background.png"), true)) {
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

    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
}

void background::deinit(void)
{
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vaobj);
    glDeleteBuffers(1, &tile_vbo);
    glDeleteBuffers(1, &cube_vbo);
    VariedProgram::destroy(tile_program);
    VariedProgram::destroy(cube_program);
}

void background::render(void)
{
    if(!VariedProgram::update(cube_program)) {
        spdlog::critical("background: cube program update failed");
        std::terminate();
    }

    if(!VariedProgram::update(tile_program)) {
        spdlog::critical("background: tile program update failed");
        std::terminate();
    }

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(vaobj);

    glViewport(0, 0, rtexture_width, rtexture_height);
    glBindFramebuffer(GL_FRAMEBUFFER, rtexture_fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if(enable_3D && (globals::gui_screen == GUI_MAIN_MENU)) {
        const float angle = 5.0f * cxpr::radians(globals::frametime);
        mdmatrix = Mat4x4f::model_rotate(mdmatrix, +1.00f * angle, Vec3f(1.0f, 0.0f, 0.0f));
        mdmatrix = Mat4x4f::model_rotate(mdmatrix, -0.33f * angle, Vec3f(0.0f, 1.0f, 0.0f));
        mdmatrix = Mat4x4f::model_rotate(mdmatrix, +0.25f * angle, Vec3f(0.0f, 0.0f, 1.0f));

        const Mat4x4f proj_1 = Mat4x4f::proj_ortho(-6.0f * globals::aspect, 0.5f * globals::aspect, -0.5f, 6.0f, -8.0f, 8.0f);
        const Mat4x4f proj_2 = Mat4x4f::proj_ortho(-0.5f * globals::aspect, 6.0f * globals::aspect, -6.0f, 0.5f, -8.0f, 8.0f);

        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vec3f), nullptr);

        glLineWidth(4.0f);

        glUseProgram(cube_program.handle);
        glUniformMatrix4fv(cube_program.uniforms[u_mdmatrix].location, 1, false, mdmatrix.data()->data());

        glUniformMatrix4fv(cube_program.uniforms[u_vpmatrix].location, 1, false, proj_1.data()->data());
        glDrawArrays(GL_LINES, 0, 24);

        glUniformMatrix4fv(cube_program.uniforms[u_vpmatrix].location, 1, false, proj_2.data()->data());
        glDrawArrays(GL_LINES, 0, 24);
    }

    const float scaled_width = 0.75f * static_cast<float>(globals::width / globals::gui_scale);
    const float scaled_height = 0.75f * static_cast<float>(globals::height / globals::gui_scale);
    const float scale_x = scaled_width / static_cast<float>(texture_width);
    const float scale_y = scaled_height / static_cast<float>(texture_height);

    glViewport(0, 0, globals::width, globals::height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rtexture);

    glUseProgram(tile_program.handle);
    glUniform2f(tile_program.uniforms[u_scale].location, scale_x, scale_y);
    glUniform1i(tile_program.uniforms[u_texture].location, 0); // GL_TEXTURE0
    glUniform1i(tile_program.uniforms[u_rtexture].location, 1); // GL_TEXTURE1

    glBindBuffer(GL_ARRAY_BUFFER, tile_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vec2f), nullptr);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
