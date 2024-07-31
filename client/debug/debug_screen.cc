// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/canvas/canvas.hh>
#include <client/canvas/font.hh>
#include <client/canvas/text.hh>
#include <client/debug/debug_screen.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

static std::string gl_version = {};
static std::string gl_renderer = {};
static Text textobj = {};

void debug_screen::init(void)
{
    gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    Text::create(textobj, 128, 1);
}

void debug_screen::deinit(void)
{
    Text::destroy(textobj);
}

void debug_screen::draw_gui(void)
{
    const auto scale = cxpr::max(1U, globals::gui_scale / 2U);

    Text::set(textobj, 0, "BIPKI");
    canvas::draw_text(16, 16, textobj, globals::font_16px);
}
