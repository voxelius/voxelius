// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/event/key.hh>
#include <client/globals.hh>
#include <client/ui_imgui.hh>
#include <client/ui_screen.hh>
#include <client/ui_settings.hh>

static void on_key(const KeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        switch(globals::ui_screen) {
            case ui::SCREEN_SETTINGS_MAIN:
                globals::ui_screen = ui::SCREEN_MAIN_MENU;
                break;
        }
    }
}

void ui::settings::init()
{

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}

void ui::settings::deinit()
{

}

void ui::settings::render_ui()
{

}
