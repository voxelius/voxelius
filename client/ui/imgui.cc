// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/mouse_button.hh>
#include <client/event/mouse_move.hh>
#include <client/globals.hh>
#include <client/ui/canvas_font.hh>
#include <client/ui/canvas_text.hh>
#include <client/ui/canvas.hh>
#include <client/ui/imgui.hh>

static int cursor_xpos = 0;
static int cursor_ypos = 0;
static uint16_t mouse_left = 0;

static void on_mouse_button(const MouseButtonEvent &event)
{
    if(event.button == GLFW_MOUSE_BUTTON_LEFT) {
        switch(event.action) {
            case GLFW_PRESS:
                mouse_left = 1;
                break;
            case GLFW_RELEASE:
                mouse_left = 0;
                break;
        }
    }
}

static void on_mouse_move(const MouseMoveEvent &event)
{
    cursor_xpos = event.xpos;
    cursor_ypos = event.ypos;
}

void imgui::init()
{
    globals::dispatcher.sink<MouseButtonEvent>().connect<&on_mouse_button>();
    globals::dispatcher.sink<MouseMoveEvent>().connect<&on_mouse_move>();
}

void imgui::update_late()
{
    if(mouse_left)
        mouse_left += 1;
}

bool imgui::button(int xpos, int ypos, const canvas::Text &text, const canvas::Font &font, const imgui::Style &style)
{
    const int iscale = static_cast<int>(globals::ui_scale);

    const int tx = iscale * (xpos + style.button_margin.x);
    const int ty = iscale * (ypos + style.button_margin.y);
    const int tw = font.get_glyph_width() * text.get_max_text_width();
    const int th = font.get_glyph_height() * text.get_max_text_height();

    const int rx = iscale * xpos;
    const int ry = iscale * ypos;
    const int rw = iscale * (2 * style.button_margin.x + tw);
    const int rh = iscale * (2 * style.button_margin.y + th);

    const int rxx = rx + rw;
    const int ryy = ry + rh;
    const bool hover = ((cursor_xpos >= rx) && (cursor_xpos < rxx) && (cursor_ypos >= ry) && (cursor_ypos < ryy));

    vector4_t background = {};
    vector4_t foreground = {};

    if(hover) {
        if(mouse_left) {
            background = style.button_background_press;
            foreground = style.button_foreground_press;
        }
        else {
            background = style.button_background_hover;
            foreground = style.button_foreground_hover;
        }
    }
    else {
        background = style.button_background;
        foreground = style.button_foreground;
    }

    canvas::rect(rx, ry, rw, rh, background);
    canvas::text(tx, ty, text, font, foreground, globals::ui_scale);

    if(hover)
        return mouse_left == 1;
    return false;
}
