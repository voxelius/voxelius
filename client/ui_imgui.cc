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
#include <limits>

static int cursor_xpos = 0;
static int cursor_ypos = 0;
static uint16_t buttons[GLFW_MOUSE_BUTTON_LAST + 1] = {};

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

    const int ss = cxmath::max(iscale, fscale / 2);
    const int sx = tx + ss;
    const int sy = ty + ss;

    canvas::draw_text(sx, sy, text, font, style.label.text_shadow, style.label.text_background, fscale);
    canvas::draw_text(tx, ty, text, font, style.label.text_foreground, style.label.text_background, fscale);
}

bool ui::imgui::button(int xpos, int ypos, int width, const canvas::Text &text, const canvas::Font &font, const ui::Style &style)
{
    const int iscale = globals::ui_scale;

    const int mx = iscale * style.button.text_margin.x;
    const int my = iscale * style.button.text_margin.y;

    const int tx = iscale * xpos + mx;
    const int ty = iscale * ypos + my;
    const int th = iscale * font.get_glyph_height() * text.get_max_text_height();

    const int rx = iscale * xpos;
    const int ry = iscale * ypos;
    const int rw = iscale * width;
    const int rh = 2 * my + th;

    const int sx = tx + iscale;
    const int sy = ty + iscale;

    const int rxx = rx + rw;
    const int ryy = ry + rh;
    const bool hover = ((cursor_xpos >= rx) && (cursor_xpos < rxx) && (cursor_ypos >= ry) && (cursor_ypos < ryy));

    vector4_t rect_col = {};
    vector4_t text_col = {};

    if(hover) {
        if(buttons[GLFW_MOUSE_BUTTON_LEFT]) {
            rect_col = style.button.rect_pressed;
            text_col = style.button.text_pressed;
        }
        else {
            rect_col = style.button.rect_hovered;
            text_col = style.button.text_hovered;
        }
    }
    else {
        rect_col = style.button.rect_default;
        text_col = style.button.text_default;
    }

    canvas::draw_rect(rx, ry, rw, rh, rect_col);
    canvas::draw_text(sx, sy, text, font, style.button.text_shadow, globals::ui_scale);
    canvas::draw_text(tx, ty, text, font, text_col, globals::ui_scale);

    if(hover)
        return buttons[GLFW_MOUSE_BUTTON_LEFT] == 1;
    return false;
}

void ui::imgui::slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style)
{
    ui::imgui::slider(xpos, ypos, width, value, text, font, style, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), 0.0);
}

void ui::imgui::slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style, double min, double max)
{
    ui::imgui::slider(xpos, ypos, width, value, text, font, style, min, max, 0.0);
}

void ui::imgui::slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style, double min, double max, double step)
{
    const int iscale = globals::ui_scale;

    const int mx = iscale * style.button.text_margin.x;
    const int my = iscale * style.button.text_margin.y;

    const int tx = iscale * xpos + mx;
    const int ty = iscale * ypos + my;
    const int th = iscale * font.get_glyph_height() * text.get_max_text_height();

    const int rx = iscale * xpos;
    const int ry = iscale * ypos;
    const int rw = iscale * width;
    const int rh = 2 * my + th;

    const int sx = tx + iscale;
    const int sy = ty + iscale;

    const int lw = iscale * font.get_glyph_width();
    const int lx = rx + (rw - lw) * ((value - min) / (max - min));

    const int rxx = rx + rw;
    const int ryy = ry + rh;
    const bool hover = ((cursor_xpos >= rx) && (cursor_xpos < rxx) && (cursor_ypos >= ry) && (cursor_ypos < ryy));

    if(hover && buttons[GLFW_MOUSE_BUTTON_LEFT]) {
        value = static_cast<double>(cursor_xpos - rx - lw / 2) / static_cast<double>(rw - lw) * (max - min) + min;
        if(step != 0.0)
            value = cxmath::floor<int>(value / step) * step;
        value = cxmath::clamp(value, min, max);
    }

    vector4_t rect_col = {};
    vector4_t text_col = {};
    vector4_t line_col = {};

    if(hover) {
        if(buttons[GLFW_MOUSE_BUTTON_LEFT]) {
            rect_col = style.slider.rect_pressed;
            text_col = style.slider.text_pressed;
            line_col = style.slider.line_pressed;
        }
        else {
            rect_col = style.slider.rect_hovered;
            text_col = style.slider.text_hovered;
            line_col = style.slider.line_hovered;
        }
    }
    else {
        rect_col = style.slider.rect_default;
        text_col = style.slider.text_default;
        line_col = style.slider.line_default;
    }

    canvas::draw_rect(rx, ry, rw, rh, rect_col);
    canvas::draw_rect(lx, ry, lw, rh, line_col);
    canvas::draw_text(sx, sy, text, font, style.button.text_shadow, globals::ui_scale);
    canvas::draw_text(tx, ty, text, font, text_col, globals::ui_scale);
}
