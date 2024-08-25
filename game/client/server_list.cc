// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/signal/dispatcher.hpp>
#include <game/client/event/glfw_key.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>
#include <game/client/server_list.hh>
#include <imgui.h>

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if((event.key == GLFW_KEY_ESCAPE) && (event.action == GLFW_PRESS)) {
        if(globals::gui_screen == GUI_SERVER_LIST) {
            globals::gui_screen = GUI_MAIN_MENU;
            return;
        }
    }
}

void server_list::init(void)
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void server_list::layout(void)
{
    ImGui::ShowStyleEditor();
}
