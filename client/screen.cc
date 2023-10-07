// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/screen.hh>
#include <GLFW/glfw3.h>
#include <shared/cxmath.hh>
#include <spdlog/spdlog.h>

static void on_framebuffer_size_event(GLFWwindow *window, int width, int height)
{
    globals::window_width = width;
    globals::window_height = height;
    globals::window_aspect = static_cast<double>(width) / static_cast<double>(height);

    globals::dispatcher.trigger(ScreenSizeEvent {
        .width = globals::window_width,
        .height = globals::window_height,
        .aspect = globals::window_aspect,
    });
}

void screen::init()
{
    spdlog::debug("screen: taking over framebuffer events");
    glfwSetFramebufferSizeCallback(globals::window, &on_framebuffer_size_event);
}

void screen::init_late()
{
    int width, height;
    glfwGetFramebufferSize(globals::window, &width, &height);
    on_framebuffer_size_event(globals::window, width, height);
}

