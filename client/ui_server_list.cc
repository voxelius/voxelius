// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/ui_screen.hh>
#include <client/ui_server_list.hh>
#include <entt/signal/dispatcher.hpp>

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

}
