// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/event/mouse_button.hh>
#include <client/event/mouse_move.hh>
#include <client/globals.hh>
#include <client/ui_imgui.hh>
#include <cwchar>

static int cursor_xpos = 0;
static int cursor_ypos = 0;
static uint16_t buttons[GLFW_MOUSE_BUTTON_LAST + 1] = {};
static canvas::Text slider_text = {};

static void on_mouse_button(const MouseButtonEvent &event)
{
    if(event.action == GLFW_PRESS) {
        // Non-zero counters are increased
        // every frame in ui::imgui::update_late
        buttons[event.button] = 1;
    }
    else {
        // Zero values are treated as if the
        // counter was stopped and wasn't supposed to increase
        buttons[event.button] = 0;
    }
}

static void on_mouse_move(const MouseMoveEvent &event)
{
    cursor_xpos = event.xpos;
    cursor_ypos = event.ypos;
}

void ui::imgui::init()
{
    globals::dispatcher.sink<MouseButtonEvent>().connect<&on_mouse_button>();
    globals::dispatcher.sink<MouseMoveEvent>().connect<&on_mouse_move>();
}

void ui::imgui::deinit()
{
    slider_text.destroy();
}

void ui::imgui::update_late()
{
    for(size_t i = 0; i <= GLFW_MOUSE_BUTTON_LAST; ++i) {
        if(!buttons[i])
            continue;
        buttons[i] += 1;
    }
}

void ui::imgui::label(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const ui::Style &style, unsigned int scale)
{
    const int iscale = globals::ui_scale;
    const int fscale = iscale * static_cast<int>(scale);

    const int tx = iscale * xpos;
    const int ty = iscale * ypos;

    const int soff = iscale * font.get_glyph_width() / 8;
    const int sx = tx + soff;
    const int sy = ty + soff;

    canvas::draw_text(sx, sy, text, font, style.label.shadow, style.label.background, fscale);
    canvas::draw_text(tx, ty, text, font, style.label.foreground, style.label.background, fscale);
}

bool ui::imgui::button(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const ui::Style &style)
{
    const int iscale = globals::ui_scale;

    const int bx = iscale * style.button.text_border.x;
    const int by = iscale * style.button.text_border.y;

    const int tx = iscale * xpos + bx;
    const int ty = iscale * ypos + by;
    const int tw = iscale * font.get_glyph_width() * text.get_max_text_width();
    const int th = iscale * font.get_glyph_height() * text.get_max_text_height();

    const int rx = iscale * xpos;
    const int ry = iscale * ypos;
    const int rw = 2 * bx + tw;
    const int rh = 2 * by + th;

    const int soff = iscale * font.get_glyph_width() / 8;
    const int sx = tx + soff;
    const int sy = ty + soff;

    const int rxx = rx + rw;
    const int ryy = ry + rh;
    const bool hover = ((cursor_xpos >= rx) && (cursor_xpos < rxx) && (cursor_ypos >= ry) && (cursor_ypos < ryy));

    vector4_t background = {};
    vector4_t foreground = {};

    if(hover) {
        if(buttons[GLFW_MOUSE_BUTTON_LEFT]) {
            background = style.button.background_pressed;
            foreground = style.button.foreground_pressed;
        }
        else {
            background = style.button.background_hovered;
            foreground = style.button.foreground_hovered;
        }
    }
    else {
        background = style.button.background_default;
        foreground = style.button.foreground_default;
    }

    canvas::draw_rect(rx, ry, rw, rh, background);
    canvas::draw_text(sx, sy, text, font, style.button.text_shadow, globals::ui_scale);
    canvas::draw_text(tx, ty, text, font, foreground, globals::ui_scale);

    if(hover)
        return buttons[GLFW_MOUSE_BUTTON_LEFT] == 1;
    return false;
}

void ui::imgui::slider(int xpos, int ypos, double &out, double min, double max, double step, const canvas::Text &text, const canvas::Font &font, const ui::Style &style)
{
    // I am working on it...
}
