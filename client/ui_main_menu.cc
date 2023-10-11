// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/debug_session.hh>
#include <client/event/keyboard_key.hh>
#include <client/globals.hh>
#include <client/ui_imgui.hh>
#include <client/ui_main_menu.hh>
#include <client/ui_screen.hh>
#include <spdlog/spdlog.h>

static ui::Style style = {};
static canvas::Text title = {};
static canvas::Text button = {};

static void on_keyboard_key(const KeyboardKeyEvent &event)
{
    if(globals::registry.valid(globals::player)) {
        if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
            switch(globals::ui_screen) {
                case ui::SCREEN_NONE:
                    globals::ui_screen = ui::SCREEN_MAIN_MENU;
                    break;
                case ui::SCREEN_MAIN_MENU:
                    globals::ui_screen = ui::SCREEN_NONE;
                    break;
            }
        }
    }
}

void ui::main_menu::init()
{
    style.label.background = COL_TRANSPARENT;
    style.label.foreground = COL_WHITE;
    style.label.shadow = {0.25, 0.25, 0.25, 1.0};

    style.button.background_default = COL_TRANSPARENT;
    style.button.background_hovered = {1.0, 1.0, 1.0, 1.0 / 16.0};
    style.button.background_pressed = {1.0, 1.0, 1.0, 1.0 / 4.0};
    style.button.foreground_default = COL_WHITE;
    style.button.foreground_hovered = COL_WHITE;
    style.button.foreground_pressed = COL_WHITE;
    style.button.text_shadow = {0.25, 0.25, 0.25, 1.0};
    style.button.text_border = vector2i_t{4, 4};

    title.create(64, 1);
    button.create(64, 1);

    globals::dispatcher.sink<KeyboardKeyEvent>().connect<&on_keyboard_key>();
}

void ui::main_menu::deinit()
{
    button.destroy();
    title.destroy();
}

void ui::main_menu::render_ui()
{
    const int xstart = (0.125 * globals::window_width) / globals::ui_scale;
    const int ystart = (0.125 * globals::window_height) / globals::ui_scale;
    const int button_height = globals::font_16px.get_glyph_height() + 2 * style.button.text_border.y;

    int ypos = ystart;

    title.set(0, L"Voxelius");
    ui::imgui::label(xstart, ypos, title, globals::font_8px, style, 4U);
    ypos += globals::font_8px.get_glyph_height() * 4U;

    title.set(0, L"Indev 0.0.1");
    ui::imgui::label(xstart, ypos, title, globals::font_8px, style, 1U);
    ypos += globals::font_8px.get_glyph_height();
    ypos += button_height;

    button.set(0, L"DEBUG SESSION");
    if(ui::imgui::button(xstart, ypos, button, globals::font_16px, style))
        debug_session::run();
    ypos += button_height;

    button.set(0, L"Join a Server");
    if(ui::imgui::button(xstart, ypos, button, globals::font_16px, style))
        globals::ui_screen = ui::SCREEN_SERVER_LIST;
    ypos += button_height;

    button.set(0, L"Settings");
    if(ui::imgui::button(xstart, ypos, button, globals::font_16px, style))
        globals::ui_screen = ui::SCREEN_SETTINGS_MAIN;
    ypos += button_height;

    button.set(0, L"Quit Game");
    if(ui::imgui::button(xstart, ypos, button, globals::font_16px, style))
        glfwSetWindowShouldClose(globals::window, true);
    ypos += button_height;
}
