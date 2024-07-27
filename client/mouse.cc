// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_cursor_pos.hh>
#include <client/globals.hh>
#include <client/mouse.hh>
#include <client/settings.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <shared/entity/head.hh>
#include <shared/util/angle.hh>
#include <shared/config.hh>

// PQ tried to bully me into using his fix for the view matrix
// going nuts when the pitch angle is 90 degrees but I forgot how it's done
constexpr static float PITCH_MIN = -1.0f * glm::radians(89.99f);
constexpr static float PITCH_MAX = +1.0f * glm::radians(89.99f);

static float previous_xpos = 0.0f;
static float previous_ypos = 0.0f;

static bool raw_input = true;
static float sensitivity = 0.25f;

static void on_glfw_cursor_pos(const GlfwCursorPosEvent &event)
{
    if(globals::ui_screen || !globals::registry.valid(globals::player)) {
        previous_xpos = event.xpos;
        previous_ypos = event.ypos;
        return;
    }

    const float dx = event.xpos - previous_xpos;
    const float dy = event.ypos - previous_ypos;

    auto &head = globals::registry.get<HeadComponent>(globals::player);
    head.angles.x -= glm::radians(dy) * sensitivity;
    head.angles.y -= glm::radians(dx) * sensitivity;
    head.angles.x = glm::clamp(head.angles.x, PITCH_MIN, PITCH_MAX);
    head.angles = util::wrap_180_n(head.angles);

    previous_xpos = event.xpos;
    previous_ypos = event.ypos;
}

void mouse::init(void)
{
    Config::add(globals::client_config, "mouse.raw_input", raw_input);
    Config::add(globals::client_config, "mouse.sensitivity", sensitivity);

    settings::add_checkbox(1, settings::MOUSE, "mouse.raw_input", raw_input, true);
    settings::add_slider(0, settings::MOUSE, "mouse.sensitivity", sensitivity, 0.05f, 0.50f, true, "%.02f");

    previous_xpos = 0.5f * static_cast<float>(globals::width);
    previous_ypos = 0.5f * static_cast<float>(globals::height);

    globals::dispatcher.sink<GlfwCursorPosEvent>().connect<&on_glfw_cursor_pos>();
}

void mouse::update_late(void)
{
    if(!globals::ui_screen && globals::registry.valid(globals::player)) {
        glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, raw_input);
    }
    else {
        glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, false);
    }
}
