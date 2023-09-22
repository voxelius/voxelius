// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/input.hh>
#include <spdlog/spdlog.h>

static void on_key_event(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    globals::dispatcher.trigger(KeyEvent {
        .key = key,
        .scancode = scancode,
        .action = action,
        .mods = mods,
    });
}

static void on_mouse_button_event(GLFWwindow *window, int button, int action, int mods)
{
    globals::dispatcher.trigger(MouseButtonEvent {
        .button = button,
        .action = action,
        .mods = mods,
    });
}

static void on_cursor_pos_event(GLFWwindow *window, double xpos, double ypos)
{
    globals::dispatcher.trigger(CursorPosEvent {
        .xpos = static_cast<float>(xpos), 
        .ypos = static_cast<float>(ypos),
    });
}

static void on_scroll_event(GLFWwindow *window, double dx, double dy)
{
    globals::dispatcher.trigger(ScrollEvent {
        .dx = static_cast<float>(dx),
        .dy = static_cast<float>(dy),
    });
}

void input::init()
{
    spdlog::debug("input: taking over input events");
    glfwSetKeyCallback(globals::window, &on_key_event);
    glfwSetMouseButtonCallback(globals::window, &on_mouse_button_event);
    glfwSetCursorPosCallback(globals::window, &on_cursor_pos_event);
    glfwSetScrollCallback(globals::window, &on_scroll_event);
}
