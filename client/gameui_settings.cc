// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gameui_settings.hh>
#include <client/glfw_events.hh>
#include <client/globals.hh>
#include <spdlog/spdlog.h>

static void on_key(const KeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        if(globals::gameui_screen == GameUiScreen::Settings) {
            globals::gameui_screen = GameUiScreen::MainMenu;
            return;
        }
    }
}

void gameui_settings::init()
{
    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}

void gameui_settings::deinit()
{

}

void gameui_settings::draw_ui()
{

}
