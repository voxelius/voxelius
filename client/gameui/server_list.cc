// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/gameui/screen.hh>
#include <client/gameui/server_list.hh>
#include <entt/signal/dispatcher.hpp>
#include <imgui.h>

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if((event.key == GLFW_KEY_ESCAPE) && (event.action == GLFW_PRESS)) {
        if(globals::ui_screen == UI_SERVER_LIST) {
            globals::ui_screen = UI_MAIN_MENU;
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
