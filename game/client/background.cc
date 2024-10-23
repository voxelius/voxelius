// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <common/fstools.hh>
#include <common/image.hh>
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_framebuffer_size.hh>
#include <game/client/background.hh>
#include <game/client/cubedraw.hh>
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

static VariedProgram tprogram = {};
static std::size_t u_t_texture = {};
static std::size_t u_t_scale = {};

static VariedProgram pprogram = {};
static std::size_t u_p_resolution = {};
static std::size_t u_p_texture = {};
static std::size_t u_p_scale = {};

static int tile_width = {};
static int tile_height = {};
static GLuint tile = {};

static int rtexture_width = {};
static int rtexture_height = {};
static GLuint rtexture_fbo = {};
static GLuint rtexture = {};

static GLuint vaobj = {};
static GLuint vbo = {};

static Mat4x4f mdmatrix = {};

static void on_glfw_framebuffer_size(const GlfwFramebufferSizeEvent &event)
{
    if(rtexture_fbo) {
        glDeleteTextures(1, &rtexture);
        glDeleteFramebuffers(1, &rtexture_fbo);
    }

    rtexture_width = event.width / 4;
    rtexture_height = event.height / 4;

    glGenTextures(1, &rtexture);
    glBindTexture(GL_TEXTURE_2D, rtexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, rtexture_width, rtexture_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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

    std::random_device random = {};
    std::uniform_real_distribution<float> dist = std::uniform_real_distribution<float>(0.0f, 90.0f);

    mdmatrix = Mat4x4f::identity();
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(1.0f, 0.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(0.0f, 1.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, dist(random), Vec3f(0.0f, 0.0f, 1.0f));

    const std::string path = std::string("textures/gui/background.png");

    Image image = {};

    if(!Image::load_rgba(image, path, true)) {
        spdlog::critical("background: {}: {}", path, fstools::error());
        std::terminate();
    }

    tile_width = image.width;
    tile_height = image.height;

    glGenTextures(1, &tile);
    glBindTexture(GL_TEXTURE_2D, tile);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tile_width, tile_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    Image::unload(image);

    if(!VariedProgram::setup(tprogram, "shaders/background_tile.vert", "shaders/background_tile.frag")) {
        spdlog::critical("background: tile program setup failed");
        std::terminate();
    }

    u_t_texture = VariedProgram::add_uniform(tprogram, "u_Texture");
    u_t_scale = VariedProgram::add_uniform(tprogram, "u_Scale");

    if(!VariedProgram::setup(pprogram, "shaders/background_pixelize.vert", "shaders/background_pixelize.frag")) {
        spdlog::critical("background: pixelize program setup failed");
        std::terminate();
    }

    u_p_resolution = VariedProgram::add_uniform(pprogram, "u_Resolution");
    u_p_texture = VariedProgram::add_uniform(pprogram, "u_Texture");
    u_p_scale = VariedProgram::add_uniform(pprogram, "u_Scale");

    const Vec2f vertices[4] = {
        Vec2f(0.0f, 1.0f),
        Vec2f(0.0f, 0.0f),
        Vec2f(1.0f, 1.0f),
        Vec2f(1.0f, 0.0f),
    };

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vec2f), nullptr);

    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
}

void background::deinit(void)
{
    glDeleteVertexArrays(1, &vaobj);
    glDeleteBuffers(1, &vbo);

    VariedProgram::destroy(pprogram);
    VariedProgram::destroy(tprogram);

    glDeleteTextures(1, &tile);
}

void background::render(void)
{
    if(!VariedProgram::update(tprogram)) {
        spdlog::critical("background: tile program update failed");
        std::terminate();
    }

    if(!VariedProgram::update(pprogram)) {
        spdlog::critical("background: pixelize program update failed");
        std::terminate();
    }

    glDisable(GL_DEPTH_TEST);

    const float angle = 5.0f * cxpr::radians(globals::frametime);
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, 1.00f * angle, Vec3f(1.0f, 0.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, 0.50f * angle, Vec3f(0.0f, 1.0f, 0.0f));
    mdmatrix = Mat4x4f::model_rotate(mdmatrix, 0.25f * angle, Vec3f(0.0f, 0.0f, 1.0f));

    if(enable_3D && (globals::gui_screen == GUI_MAIN_MENU)) {
        glViewport(0, 0, rtexture_width, rtexture_height);
        glBindFramebuffer(GL_FRAMEBUFFER, rtexture_fbo);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        cubedraw::render(mdmatrix, Mat4x4f::proj_ortho(-6.0f * globals::aspect, 0.5f * globals::aspect, -0.5f, 6.0f, -8.0f, 8.0f), 0.5f * globals::gui_scale, Vec4f::white());
        cubedraw::render(mdmatrix, Mat4x4f::proj_ortho(-0.5f * globals::aspect, 6.0f * globals::aspect, -6.0f, 0.5f, -8.0f, 8.0f), 0.5f * globals::gui_scale, Vec4f::white());

        glViewport(0, 0, globals::width, globals::height);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, rtexture);
        glActiveTexture(GL_TEXTURE0);

        glUseProgram(pprogram.handle);
        glUniform2f(pprogram.uniforms[u_p_resolution].location, globals::width, globals::height);
        glUniform1f(pprogram.uniforms[u_p_scale].location, 4.0f * globals::gui_scale);
        glUniform1i(pprogram.uniforms[u_p_texture].location, 0); // GL_TEXTURE0

        glBindVertexArray(vaobj);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    else {
        const float scaled_width = 0.75f * static_cast<float>(globals::width / globals::gui_scale);
        const float scaled_height = 0.75f * static_cast<float>(globals::height / globals::gui_scale);
        const float scale_x = scaled_width / static_cast<float>(tile_width);
        const float scale_y = scaled_height / static_cast<float>(tile_height);

        glBindTexture(GL_TEXTURE_2D, tile);
        glActiveTexture(GL_TEXTURE0);

        glUseProgram(tprogram.handle);
        glUniform2f(tprogram.uniforms[u_t_scale].location, scale_x, scale_y);
        glUniform1i(tprogram.uniforms[u_t_texture].location, 0); // GL_TEXTURE0

        glBindVertexArray(vaobj);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}
