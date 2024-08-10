// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/background.hh>
#include <client/globals.hh>
#include <client/settings.hh>
#include <client/varied_program.hh>
#include <GLFW/glfw3.h>
#include <shared/math/constexpr.hh>
#include <shared/math/vec2f.hh>
#include <shared/util/physfs.hh>
#include <shared/config.hh>
#include <shared/image.hh>
#include <spdlog/spdlog.h>

static bool enable_panorama = false;

static VariedProgram program = {};
static std::size_t u_screen_scale = {};
static std::size_t u_curtime = {};
static std::size_t u_texture = {};
static int texture_width = {};
static int texture_height = {};
static GLuint texture = {};
static GLuint vaobj = {};
static GLuint vbo = {};

void background::init(void)
{
    Config::add(globals::client_config, "background.panorama", enable_panorama);

    settings::add_checkbox(0, settings::VIDEO_GUI, "background.panorama", enable_panorama, true);

    const std::string path = std::string("textures/gui/background.png");

    Image image = {};

    if(!Image::load_rgba(image, path, true)) {
        //spdlog::critical("background: {}: {}", path, util::physfs_error());
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

    if(!VariedProgram::setup(program, "shaders/background.vert", "shaders/background.frag")) {
        spdlog::critical("background: program setup failed");
        std::terminate();
    }

    u_screen_scale = VariedProgram::add_uniform(program, "u_ScreenScale");
    u_curtime = VariedProgram::add_uniform(program, "u_CurTime");
    u_texture = VariedProgram::add_uniform(program, "u_Texture");

    const Vec2f vertices[4] = {
        Vec2f(-1.0f,  1.0f),
        Vec2f(-1.0f, -1.0f),
        Vec2f( 1.0f,  1.0f),
        Vec2f( 1.0f, -1.0f),
    };

    glGenVertexArrays(1, &vaobj);
    glBindVertexArray(vaobj);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vec2f), nullptr);
}

void background::deinit(void)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vaobj);
    VariedProgram::destroy(program);
    glDeleteTextures(1, &texture);
}

void background::render(void)
{
    if(!VariedProgram::update(program)) {
        spdlog::critical("background: program update failed");
        VariedProgram::destroy(program);
        std::terminate();
    }

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisable(GL_DEPTH_TEST);

    const float scaled_width = 0.75f * static_cast<float>(globals::width / globals::gui_scale);
    const float scaled_height = 0.75f * static_cast<float>(globals::height / globals::gui_scale);
    const float scale_x = scaled_width / static_cast<float>(texture_width);
    const float scale_y = scaled_height / static_cast<float>(texture_height);

    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0);

    glUseProgram(program.handle);
    glUniform2f(program.uniforms[u_screen_scale].location, scale_x, scale_y);
    glUniform1f(program.uniforms[u_curtime].location, glfwGetTime());
    glUniform1i(program.uniforms[u_texture].location, 0); // GL_TEXTURE0

    glBindVertexArray(vaobj);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
