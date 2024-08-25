// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/client/entity/player_look.hh>
#include <game/client/event/glfw_cursor_pos.hh>
#include <game/client/globals.hh>
#include <game/client/mouse.hh>
#include <game/client/settings.hh>
#include <GLFW/glfw3.h>
#include <mathlib/constexpr.hh>

static float previous_xpos = 0.0f;
static float previous_ypos = 0.0f;

bool mouse::enable_raw_input = true;
float mouse::sensitivity = 0.25f;

static void on_glfw_cursor_pos(const GlfwCursorPosEvent &event)
{
    if(globals::gui_screen || !globals::registry.valid(globals::player)) {
        previous_xpos = event.xpos;
        previous_ypos = event.ypos;
        return;
    }

    const float dx = -1.0f * mouse::sensitivity * cxpr::radians(event.xpos - previous_xpos);
    const float dy = -1.0f * mouse::sensitivity * cxpr::radians(event.ypos - previous_ypos);
    player_look::add_angles(dy, dx);

    previous_xpos = event.xpos;
    previous_ypos = event.ypos;
}

void mouse::init(void)
{
    Config::add(globals::client_config, "mouse.enable_raw_input", mouse::enable_raw_input);
    Config::add(globals::client_config, "mouse.sensitivity", mouse::sensitivity);

    settings::add_checkbox(1, settings::MOUSE, "mouse.enable_raw_input", mouse::enable_raw_input, true);
    settings::add_slider(0, settings::MOUSE, "mouse.sensitivity", mouse::sensitivity, 0.05f, 0.50f, true, "%.02f");

    previous_xpos = 0.5f * static_cast<float>(globals::width);
    previous_ypos = 0.5f * static_cast<float>(globals::height);

    globals::dispatcher.sink<GlfwCursorPosEvent>().connect<&on_glfw_cursor_pos>();
}

void mouse::update_late(void)
{
    if(!globals::gui_screen && globals::registry.valid(globals::player)) {
        glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, mouse::enable_raw_input);
    }
    else {
        glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, false);
    }
}
