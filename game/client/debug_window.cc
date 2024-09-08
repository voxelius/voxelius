// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_key.hh>
#include <game/client/debug_window.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <imgui.h>

// Things that have layout_debug
#include <game/client/skybox.hh>

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.action == GLFW_PRESS) {
        if((event.key == GLFW_KEY_ESCAPE) && (globals::gui_screen == GUI_DEBUG_WINDOW)) {
            globals::gui_screen = GUI_SCREEN_NONE;
            return;
        }
    }
}

void debug_window::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void debug_window::layout(void)
{
    ImGui::PushFont(globals::font_debug);
    ImGui::SetNextWindowFocus();

    if(ImGui::Begin("Debug###debug_window", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if(ImGui::BeginTabBar("###debug_window.tabs")) {
            if(ImGui::BeginTabItem("Skybox")) {
                skybox::layout_debug();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();
    ImGui::PopFont();
}

void debug_window::toggle(void)
{
    if(globals::gui_screen == GUI_SCREEN_NONE) {
        globals::gui_screen = GUI_DEBUG_WINDOW;
        return;
    }

    if(globals::gui_screen == GUI_DEBUG_WINDOW) {
        globals::gui_screen = GUI_SCREEN_NONE;
        return;
    }
}
