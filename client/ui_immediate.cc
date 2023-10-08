// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/input.hh>
#include <client/ui_color.hh>
#include <client/ui_draw.hh>
#include <client/ui_font.hh>
#include <client/ui_immediate.hh>
#include <client/ui_text.hh>
#include <spdlog/spdlog.h>

static int cursor_xpos = 0;
static int cursor_ypos = 0;
static uint64_t mouse_left = 0;

static void on_cursor_pos(const CursorPosEvent &event)
{
    cursor_xpos = event.xpos;
    cursor_ypos = event.ypos;
}

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

void ui::im::init()
{
    globals::dispatcher.sink<CursorPosEvent>().connect<&on_cursor_pos>();
    globals::dispatcher.sink<MouseButtonEvent>().connect<&on_mouse_button>();
}

void ui::im::update_late()
{
    if(mouse_left)
        mouse_left += 1;
}

bool ui::im::button(int xpos, int ypos, const ui::Text &text, const ui::Font &font, const ui::im::Style &style)
{
    const int iscale = static_cast<int>(style.element_scale);

    const int tx = xpos + style.button_margin.x;
    const int ty = ypos + style.button_margin.y;
    const int tw = font.get_glyph_width() * text.get_max_text_width();
    const int th = font.get_glyph_height() * text.get_max_text_height();

    const int rx = iscale * xpos;
    const int ry = iscale * ypos;
    const int rw = iscale * (2 * style.button_margin.x + tw);
    const int rh = iscale * (2 * style.button_margin.y + th);

    const int rxx = rx + rw;
    const int ryy = ry + rh;
    const bool hover = ((cursor_xpos >= rx) && (cursor_xpos <= rxx) && (cursor_ypos >= ry) && (cursor_ypos <= ryy));

    vector4_t background = {};
    vector4_t foreground = {};

    if(hover) {
        if(mouse_left) {
            background = style.button_background[ui::im::BUTTON_PRESS];
            foreground = style.button_foreground[ui::im::BUTTON_PRESS];
        }
        else {
            background = style.button_background[ui::im::BUTTON_HOVER];
            foreground = style.button_foreground[ui::im::BUTTON_HOVER];
        }
    }
    else {
        background = style.button_background[ui::im::BUTTON_IDLE];
        foreground = style.button_foreground[ui::im::BUTTON_IDLE];
    }

    ui::draw::rect(rx, ry, rw, rh, background);
    ui::draw::text(tx, ty, text, font, foreground, style.element_scale);

    if(hover)
        return mouse_left == 1;
    return false;
}
