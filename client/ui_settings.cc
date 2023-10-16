// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/camera.hh>
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/event/key.hh>
#include <client/globals.hh>
#include <client/player_look.hh>
#include <client/ui_imgui.hh>
#include <client/ui_screen.hh>
#include <client/ui_settings.hh>
#include <entt/signal/dispatcher.hpp>
#include <shared/config/boolean.hh>
#include <shared/config/number.hh>
#include <shared/config/string.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/xchar.h>
#include <vector>

static int category_page = 0;
static int category_select = 0;
static std::vector<std::wstring> category_names = {};
static std::vector<std::vector<void(*)(int xpos, int ypos, int width, int ystep)>> category_draw = {};

static ui::Style style = {};
static canvas::Text text = {};

static void on_key(const KeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        if(globals::ui_screen == ui::SCREEN_SETTINGS) {
            globals::ui_screen = ui::SCREEN_MAIN_MENU;
            category_select = 0U;
            category_page = 0U;
        }
    }
}

static void draw_general(int xpos, int ypos, int width, int ystep)
{
    text.set(0, fmt::format(L"FOV: {:.0f}", camera::fov.value));
    ui::imgui::slider(xpos, ypos, width, camera::fov.value, text, globals::font_16px, style, 54.0, 110.0, 1.0);
    ypos += ystep;
}

static void draw_controls_mouse(int xpos, int ypos, int width, int ystep)
{
    text.set(0, fmt::format("Raw input: {}", player_look::raw_input.value));
    if(ui::imgui::button(xpos, ypos, width, text, globals::font_16px, style))
        player_look::raw_input.value = !player_look::raw_input.value;
    ypos += ystep;

    text.set(0, fmt::format(L"Sensitivity: {:.02f}", player_look::sensitivity.value));
    ui::imgui::slider(xpos, ypos, width, player_look::sensitivity.value, text, globals::font_16px, style, 0.05, 0.50, 0.01);
    ypos += ystep;
}

static void draw_controls_keyboard(int xpos, int ypos, int width, int ystep)
{

}

static void draw_graphics(int xpos, int ypos, int width, int ystep)
{
    double view_distance_v = camera::view_distance.value;
    text.set(0, fmt::format("View distance: {}", camera::view_distance.value));
    if(ui::imgui::slider(xpos, ypos, width, view_distance_v, text, globals::font_16px, style, 1.0, 32.0, 1.0))
        camera::view_distance.value = view_distance_v;
    ypos += ystep;
}

static void draw_sound(int xpos, int ypos, int width, int ystep)
{
    text.set(0, L"There's no sound yet");
    ui::imgui::label(xpos, ypos, text, globals::font_16px, style, 1U);
    ypos += globals::font_16px.get_glyph_height();
}

void ui::settings::init()
{
    category_names.resize(4U);
    category_draw.resize(4U);

    category_names[0] = L"General";
    category_draw[0].push_back(&draw_general);

    category_names[1] = L"Controls";
    category_draw[1].push_back(&draw_controls_mouse);
    category_draw[1].push_back(&draw_controls_keyboard);

    category_names[2] = L"Graphics";
    category_draw[2].push_back(&draw_graphics);

    category_names[3] = L"Sound";
    category_draw[3].push_back(&draw_sound);

    style.rect_default = {0.0, 0.0, 0.0, 0.5};
    style.rect_hovered = {0.5, 0.5, 0.5, 0.5};
    style.rect_pressed = {0.5, 0.5, 0.5, 1.0};
    style.rect_text_padding = glm::ivec2{4, 2};
    style.text_shadow = {0.25, 0.25, 0.25, 0.5};
    style.slider_default = {0.8, 0.8, 0.8, 1.0};
    style.slider_hovered = {0.8, 0.8, 0.8, 1.0};
    style.slider_pressed = {0.1, 0.1, 0.1, 1.0};

    text.create(64, 1);

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}

void ui::settings::deinit()
{
    text.destroy();
}

void ui::settings::render()
{
    int xpos;
    int ypos;

    const int xstart = globals::width / 16 / globals::ui_scale;
    const int ystart = globals::height / 16 / globals::ui_scale;

    xpos = xstart;
    ypos = ystart;

    const int csel_width = 110;
    const int csel_height = globals::font_16px.get_glyph_height() + 2 * style.rect_text_padding.y;
    const int csel_xstep = csel_width + 8;
    const int csel_ystep = csel_height + 8;

    text.set(0, L"<< Back");
    if(ui::imgui::button(xpos, ypos, csel_width, text, globals::font_16px, style)) {
        globals::ui_screen = ui::SCREEN_MAIN_MENU;
        category_select = 0U;
        category_page = 0U;
        return;
    }

    ypos += csel_ystep + 8;

    for(int i = 0; i < category_names.size(); ++i) {
        text.set(0, category_names[i]);

        if(ui::imgui::button(xpos, ypos, csel_width, text, globals::font_16px, style)) {
            category_select = i;
            category_page = 0;
        }

        ypos += csel_ystep;
    }

    xpos = xstart + csel_xstep;
    ypos = ystart;

    const int page_count = category_draw[category_select].size();
    const int psel_width = 2 * (globals::font_16px.get_glyph_width() + style.rect_text_padding.x);
    const int psel_height = csel_height;
    const int psel_xstep = psel_width + 2;
    const int psel_ystep = psel_height + 16;

    text.set(0, L"<<");
    if(ui::imgui::button(xpos, ypos, psel_width, text, globals::font_16px, style))
        category_page -= 1;
    xpos += psel_xstep;

    text.set(0, L">>");
    if(ui::imgui::button(xpos, ypos, psel_width, text, globals::font_16px, style))
        category_page += 1;
    xpos += psel_xstep;

    category_page = cxmath::clamp(category_page, 0, page_count - 1);

    const int tx = xpos + style.rect_text_padding.x;
    const int ty = ypos + style.rect_text_padding.y;
    text.set(0, fmt::format(L"{}/{}", category_page + 1, page_count));
    ui::imgui::label(tx, ty, text, globals::font_16px, style, 1U);
    xpos += psel_xstep;

    xpos = xstart + csel_xstep;
    ypos = ystart + psel_ystep;

    const int val_width = globals::width / globals::ui_scale - 2 * xstart - csel_xstep;
    const int val_ystep = csel_height + 2;

    category_draw[category_select][category_page](xpos, ypos, val_width, val_ystep);
}
