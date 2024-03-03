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
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/ui_screen.hh>
#include <client/ui_server_list.hh>
#include <entt/signal/dispatcher.hpp>
#include <imgui.h>

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        switch(globals::ui_screen) {
            case ui::SCREEN_SERVER_LIST:
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
                break;
        }
    }
}

void ui::server_list::init()
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void ui::server_list::layout()
{
    ImGui::ShowDemoWindow();
}
