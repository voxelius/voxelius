// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/event/keyboard_key.hh>
#include <client/globals.hh>
#include <client/ui_imgui.hh>
#include <client/ui_screen.hh>
#include <client/ui_settings.hh>
#include <shared/config.hh>
#include <client/player_look.hh>
#include <client/camera.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/xchar.h>

constexpr static const int CATEGORY_GENERAL = 0;
constexpr static const int CATEGORY_CONTROLS = 1;
constexpr static const int CATEGORY_GRAPHICS = 2;
constexpr static const int CATEGORY_SOUND = 3;
constexpr static const int NUM_CATEGORIES = 4;

using draw_page_t = void(*)(int xpos, int ypos, int width, int ystep);

static std::wstring category_names[NUM_CATEGORIES] = {};
static int category_pages[NUM_CATEGORIES] = {};
static draw_page_t category_draws[NUM_CATEGORIES] = {};

static ui::Style style = {};
static canvas::Text text = {};
static int category_index = 0;
static int category_page = 0;

static void close_screen()
{
    globals::ui_screen = ui::SCREEN_MAIN_MENU;
    category_index = 0;
    category_page = 0;
}

static void on_keyboard_key(const KeyboardKeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        switch(globals::ui_screen) {
            case ui::SCREEN_SETTINGS:
                close_screen();
                break;
        }
    }
}

static void draw_general(int xpos, int ypos, int width, int ystep)
{
    text.set(0, L"Nothing to see here");
    ui::imgui::label(xpos, ypos, text, globals::font_16px, style, 1U);
    ypos += globals::font_16px.get_glyph_height();

    text.set(0, fmt::format(L"WIP for {} years", globals::curtime / 1000000));
    ui::imgui::label(xpos, ypos, text, globals::font_16px, style, 1U);
    ypos += globals::font_16px.get_glyph_height();
}

static void draw_controls(int xpos, int ypos, int width, int ystep)
{
    if(category_page == 0) {
        bool raw_input = player_look::raw_input.get_value();
        text.set(0, fmt::format(L"Raw input: {}", raw_input));
        if(ui::imgui::button(xpos, ypos, width, text, globals::font_16px, style))
            raw_input = !raw_input;
        player_look::raw_input.set_value(raw_input);
        ypos += ystep;

        double sensitivity = player_look::sensitivity.get_value();
        text.set(0, fmt::format(L"Sensitivity: {:.02f}", sensitivity));
        if(ui::imgui::slider(xpos, ypos, width, sensitivity, text, globals::font_16px, style, 0.05, 0.5, 0.01))
            player_look::sensitivity.set_value(sensitivity);
        ypos += ystep;
    }
}

static void draw_graphics(int xpos, int ypos, int width, int ystep)
{
    if(category_page == 0) {
        double fov = camera::fov.get_value();
        text.set(0, fmt::format("FOV: {:.0f}", fov));
        if(ui::imgui::slider(xpos, ypos, width, fov, text, globals::font_16px, style, 54, 120, 1))
            camera::fov.set_value(fov);
        ypos += ystep;

        double view_distance = camera::view_distance.get_value();
        text.set(0, fmt::format("View distance: {}", view_distance));
        if(ui::imgui::slider(xpos, ypos, width, view_distance, text, globals::font_16px, style, 1, 32, 1))
            camera::view_distance.set_value(view_distance);
        ypos += ystep;
    }
}

static void draw_sound(int xpos, int ypos, int width, int ystep)
{
    text.set(0, L"There's no sound yet");
    ui::imgui::label(xpos, ypos, text, globals::font_16px, style, 1U);
    ypos += globals::font_16px.get_glyph_height();
}

void ui::settings::init()
{
    category_names[CATEGORY_GENERAL] = L"General";
    category_pages[CATEGORY_GENERAL] = 1;
    category_draws[CATEGORY_GENERAL] = &draw_general;

    category_names[CATEGORY_CONTROLS] = L"Controls";
    category_pages[CATEGORY_CONTROLS] = 2;
    category_draws[CATEGORY_CONTROLS] = &draw_controls;

    category_names[CATEGORY_GRAPHICS] = L"Graphics";
    category_pages[CATEGORY_GRAPHICS] = 1;
    category_draws[CATEGORY_GRAPHICS] = &draw_graphics;

    category_names[CATEGORY_SOUND] = L"Sound";
    category_pages[CATEGORY_SOUND] = 1;
    category_draws[CATEGORY_SOUND] = &draw_sound;

    style.rect_default = {0.0, 0.0, 0.0, 0.5};
    style.rect_hovered = {0.5, 0.5, 0.5, 0.5};
    style.rect_pressed = {0.5, 0.5, 0.5, 1.0};
    style.rect_text_padding = vector2i_t{4, 2};
    style.text_shadow = {0.25, 0.25, 0.25, 0.5};
    style.slider_default = {0.8, 0.8, 0.8, 1.0};
    style.slider_hovered = {0.8, 0.8, 0.8, 1.0};
    style.slider_pressed = {0.1, 0.1, 0.1, 1.0};

    text.create(64, 1);

    globals::dispatcher.sink<KeyboardKeyEvent>().connect<&on_keyboard_key>();
}

void ui::settings::deinit()
{
    text.destroy();
}

void ui::settings::render_ui()
{
    int xpos;
    int ypos;

    const int xstart = globals::window_width / 16 / globals::ui_scale;
    const int ystart = globals::window_height / 16 / globals::ui_scale;

    const int csel_width = 110;
    const int csel_height = globals::font_16px.get_glyph_height() + 2 * style.rect_text_padding.y;
    const int csel_xstep = csel_width + 8;
    const int csel_ystep = csel_height + 8;

    const int psel_width = 2 * (globals::font_16px.get_glyph_width() + style.rect_text_padding.x);
    const int psel_height = csel_height;
    const int psel_xstep = psel_width + 2;
    const int psel_ystep = psel_height + 16;

    const int val_width = globals::window_width / globals::ui_scale - 2 * xstart - csel_xstep;
    const int val_ystep = csel_height + 2;


    //
    // CATEGORY SELECTORS
    //

    xpos = xstart;
    ypos = ystart;

    text.set(0, L"<< Back");
    if(ui::imgui::button(xpos, ypos, csel_width, text, globals::font_16px, style))
        close_screen();
    ypos += csel_ystep + 8;

    for(int i = 0; i < NUM_CATEGORIES; ++i) {
        if(i == category_index)
            text.set(0, fmt::format(L"> {}", category_names[i]));
        else text.set(0, category_names[i]);

        if(ui::imgui::button(xpos, ypos, csel_width, text, globals::font_16px, style)) {
            category_index = i;
            category_page = 0;
        }

        ypos += csel_ystep;
    }


    //
    // PAGE SELECTOR
    //

    xpos = xstart + csel_xstep;
    ypos = ystart;

    text.set(0, L"<<");
    if(ui::imgui::button(xpos, ypos, psel_width, text, globals::font_16px, style))
        category_page -= 1;
    xpos += psel_xstep;

    text.set(0, L">>");
    if(ui::imgui::button(xpos, ypos, psel_width, text, globals::font_16px, style))
        category_page += 1;
    xpos += psel_xstep;

    const int page_count = category_pages[category_index];
    category_page = cxmath::clamp(category_page, 0, page_count - 1);

    const int tx = xpos + style.rect_text_padding.x;
    const int ty = ypos + style.rect_text_padding.y;
    text.set(0, fmt::format(L"Page: {}/{}", category_page + 1, page_count));
    ui::imgui::label(tx, ty, text, globals::font_16px, style, 1U);
    xpos += psel_xstep;


    //
    // PAGE ITSELF
    //

    xpos = xstart + csel_xstep;
    ypos = ystart + psel_ystep;

    if(!category_draws[category_index])
        return;
    category_draws[category_index](xpos, ypos, val_width, val_ystep);
}

