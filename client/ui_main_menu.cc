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
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

static ui::Style style = {};
static canvas::Text text = {};

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
    style.rect_default = {0.0, 0.0, 0.0, 0.5};
    style.rect_hovered = {0.5, 0.5, 0.5, 0.5};
    style.rect_pressed = {0.5, 0.5, 0.5, 1.0};
    style.rect_text_padding = glm::ivec2{4, 2};
    style.text_shadow = {0.25, 0.25, 0.25, 0.5};

    text.create(64, 1);

    globals::dispatcher.sink<KeyboardKeyEvent>().connect<&on_keyboard_key>();
}

void ui::main_menu::deinit()
{
    text.destroy();
}

void ui::main_menu::render()
{
    int xpos;
    int ypos;

    const int btn_width = 196;
    const int btn_height = globals::font_16px.get_glyph_height() + 2 * style.rect_text_padding.y;
    const int btn_ystep = btn_height + 8;

    const int xstart = globals::width / 16 / globals::ui_scale;
    const int ystart = globals::height / 16 / globals::ui_scale;

    xpos = xstart;
    ypos = ystart;

    text.set(0, L"Voxelius");
    ui::imgui::label(xpos, ypos, text, globals::font_8px, style, 4U);
    ypos += globals::font_8px.get_glyph_height() * 4.5;

    text.set(0, L"Indev 0.0.1");
    ui::imgui::label(xpos, ypos, text, globals::font_8px, style, 1U);
    ypos += globals::font_8px.get_glyph_height();
    ypos += btn_ystep;

    text.set(0, L"DEBUG SESSION");
    if(ui::imgui::button(xpos, ypos, btn_width, text, globals::font_16px, style))
        debug_session::run();
    ypos += btn_ystep;

    text.set(0, L"Join a Server");
    if(ui::imgui::button(xpos, ypos, btn_width, text, globals::font_16px, style))
        globals::ui_screen = ui::SCREEN_SERVER_LIST;
    ypos += btn_ystep;

    text.set(0, L"Settings");
    if(ui::imgui::button(xpos, ypos, btn_width, text, globals::font_16px, style))
        globals::ui_screen = ui::SCREEN_SETTINGS;
    ypos += btn_ystep;

    text.set(0, L"Quit Game");
    if(ui::imgui::button(xpos, ypos, btn_width, text, globals::font_16px, style))
        glfwSetWindowShouldClose(globals::window, true);
    ypos += btn_ystep;
}
