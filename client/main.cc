// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <client/event/glfw_cursor_pos.hh>
#include <client/event/glfw_key.hh>
#include <client/event/glfw_mouse_button.hh>
#include <client/event/glfw_scroll.hh>
#include <client/event/glfw_framebuffer_size.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/image.hh>
#include <client/main.hh>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <shared/cmdline.hh>
#include <shared/epoch.hh>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#if defined(_WIN32)
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

static void on_glfw_error(int code, const char *message)
{
    spdlog::error("glfw: {}", message);
}

static void on_glfw_char(GLFWwindow *window, unsigned int codepoint)
{
    // TODO: imgui passthrough
}

static void on_glfw_cursor_enter(GLFWwindow *window, int entered)
{
    // TODO: imgui passthrough
}

static void on_glfw_cursor_pos(GLFWwindow *window, double xpos, double ypos)
{
    GlfwCursorPosEvent event = {};
    event.xpos = xpos;
    event.ypos = ypos;
    globals::dispatcher.trigger(event);
}

static void on_glfw_framebuffer_size(GLFWwindow *window, int width, int height)
{
    globals::width = width;
    globals::height = height;
    globals::aspect = static_cast<double>(width) / static_cast<double>(height);

    GlfwFramebufferSizeEvent fb_event = {};
    fb_event.width = globals::width;
    fb_event.height = globals::height;
    fb_event.aspect = globals::aspect;
    globals::dispatcher.trigger(fb_event);
}

static void on_glfw_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    GlfwKeyEvent event = {};
    event.key = key;
    event.scancode = scancode;
    event.action = action;
    event.mods = mods;
    globals::dispatcher.trigger(event);
}

static void on_glfw_monitor_event(GLFWmonitor *monitor, int event)
{
    ImGui_ImplGlfw_MonitorCallback(monitor, event);
}

static void on_glfw_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
    GlfwMouseButtonEvent event = {};
    event.button = button;
    event.action = action;
    event.mods = mods;
    globals::dispatcher.trigger(event);
}

static void on_glfw_scroll(GLFWwindow *window, double dx, double dy)
{
    GlfwScrollEvent event = {};
    event.dx = dx;
    event.dx = dy;
    globals::dispatcher.trigger(event);
}

static void on_glfw_window_focus(GLFWwindow *window, int focused)
{
    ImGui_ImplGlfw_WindowFocusCallback(window, focused);
}

void client::main(void)
{
    glfwSetErrorCallback(&on_glfw_error);

    if(!glfwInit()) {
        spdlog::critical("glfw: init failed");
        std::terminate();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 0);

    globals::window = glfwCreateWindow(854, 480, "Client", nullptr, nullptr);

    if(!globals::window) {
        spdlog::critical("glfw: failed to open a window");
        std::terminate();
    }

    glfwSetWindowSizeLimits(globals::window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwSetCharCallback(globals::window, &on_glfw_char);
    glfwSetCursorEnterCallback(globals::window, &on_glfw_cursor_enter);
    glfwSetCursorPosCallback(globals::window, &on_glfw_cursor_pos);
    glfwSetFramebufferSizeCallback(globals::window, &on_glfw_framebuffer_size);
    glfwSetKeyCallback(globals::window, &on_glfw_key);
    glfwSetMouseButtonCallback(globals::window, &on_glfw_mouse_button);
    glfwSetScrollCallback(globals::window, &on_glfw_scroll);
    glfwSetWindowFocusCallback(globals::window, &on_glfw_window_focus);

    glfwSetMonitorCallback(&on_glfw_monitor_event);

    Image image = {};
    GLFWimage icon = {};

    if(Image::load_rgba(image, "32x32.png", false)) {
        icon.width = image.width;
        icon.height = image.height;
        icon.pixels = reinterpret_cast<unsigned char *>(image.data);
        glfwSetWindowIcon(globals::window, 1, &icon);

        Image::unload(image);
    }

    glfwMakeContextCurrent(globals::window);
    glfwSwapInterval(1);

    if(!gladLoadGL(&glfwGetProcAddress)) {
        spdlog::critical("glad: failed to load function pointers");
        std::terminate();
    }

    spdlog::info("opengl: version: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
    spdlog::info("opengl: renderer: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(globals::window, false);
    ImGui_ImplOpenGL3_Init(nullptr);

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    globals::frametime = 0.0;
    globals::curtime = epoch::microseconds();
    globals::framecount = 0;

    client_game::init();

    int wwidth, wheight;
    glfwGetFramebufferSize(globals::window, &wwidth, &wheight);
    on_glfw_framebuffer_size(globals::window, wwidth, wheight);

    client_game::init_late();

    uint64_t last_curtime = globals::curtime;

    while(!glfwWindowShouldClose(globals::window)) {
        globals::curtime = epoch::microseconds();
        globals::frametime = static_cast<double>(globals::curtime - last_curtime) / 1000000.0;
        last_curtime = globals::curtime;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        client_game::update();

        glDisable(GL_BLEND);
        glUseProgram(0);

        client_game::render();

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, globals::width, globals::height);

        client_game::layout();

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(globals::window);

        client_game::update_late();

        glfwPollEvents();

        globals::dispatcher.update();

        globals::framecount += 1;
    }

    spdlog::info("client: shutdown after {} frames", globals::framecount);

    client_game::shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(globals::window);
    glfwTerminate();
}
