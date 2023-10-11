// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/debug_session.hh>
#include <client/event/key.hh>
#include <client/globals.hh>
#include <client/ui_imgui.hh>
#include <client/ui_main_menu.hh>
#include <client/ui_screen.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/xchar.h>
#include <spdlog/spdlog.h>

static ui::Style style = {};
static canvas::Text title = {};
static canvas::Text button = {};

static void on_key(const KeyEvent &event)
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
    style.label.text_background = COL_TRANSPARENT;
    style.label.text_foreground = COL_WHITE;
    style.label.text_shadow = {0.25, 0.25, 0.25, 1.0};

    style.button.rect_default = COL_TRANSPARENT;
    style.button.rect_hovered = {1.0, 1.0, 1.0, 1.0 / 16.0};
    style.button.rect_pressed = {1.0, 1.0, 1.0, 1.0 / 4.0};
    style.button.text_default = COL_WHITE;
    style.button.text_hovered = COL_WHITE;
    style.button.text_pressed = COL_WHITE;
    style.button.text_shadow = {0.25, 0.25, 0.25, 1.0};
    style.button.text_margin = vector2i_t{2, 2};

    style.slider.rect_default = COL_TRANSPARENT;
    style.slider.rect_hovered = {1.0, 1.0, 1.0, 1.0 / 16.0};
    style.slider.rect_pressed = {1.0, 1.0, 1.0, 1.0 / 4.0};
    style.slider.text_default = COL_WHITE;
    style.slider.text_hovered = COL_WHITE;
    style.slider.text_pressed = COL_WHITE;
    style.slider.text_shadow = {0.25, 0.25, 0.25, 1.0};
    style.slider.line_default = COL_LIGHT_GRAY;
    style.slider.line_hovered = COL_LIGHT_GRAY;
    style.slider.line_pressed = COL_LIGHT_GRAY;
    style.slider.text_margin = vector2i_t{2, 2};

    title.create(64, 1);
    button.create(64, 1);

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
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
    const int button_height = globals::font_16px.get_glyph_height() + 2 * style.button.text_margin.y;
    const int button_width = 128;

    int ypos = ystart;

    title.set(0, L"Voxelius");
    ui::imgui::label(xstart, ypos, title, globals::font_8px, style, 4U);
    ypos += globals::font_8px.get_glyph_height() * 4.5;

    title.set(0, L"Indev 0.0.1");
    ui::imgui::label(xstart, ypos, title, globals::font_8px, style, 1U);
    ypos += globals::font_8px.get_glyph_height();
    ypos += button_height / 2;

    button.set(0, L"DEBUG SESSION");
    if(ui::imgui::button(xstart, ypos, button_width, button, globals::font_16px, style))
        debug_session::run();
    ypos += button_height;

    button.set(0, L"Join a Server");
    if(ui::imgui::button(xstart, ypos, button_width, button, globals::font_16px, style))
        globals::ui_screen = ui::SCREEN_SERVER_LIST;
    ypos += button_height;

    button.set(0, L"Settings");
    if(ui::imgui::button(xstart, ypos, button_width, button, globals::font_16px, style))
        globals::ui_screen = ui::SCREEN_SETTINGS_MAIN;
    ypos += button_height;

    button.set(0, L"Quit Game");
    if(ui::imgui::button(xstart, ypos, button_width, button, globals::font_16px, style))
        glfwSetWindowShouldClose(globals::window, true);
    ypos += button_height;

    // Test
    static double value = 90.0;
    button.set(0, fmt::format(L"FOV: {}", static_cast<int>(value)));
    ui::imgui::slider(xstart, ypos, button_width, value, button, globals::font_16px, style, 60, 120, 1);
    ypos += button_height;
}
