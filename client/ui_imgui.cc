// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/canvas.hh>
#include <client/event/cursor_move.hh>
#include <client/event/mouse_button.hh>
#include <client/event/mouse_scroll.hh>
#include <client/globals.hh>
#include <client/ui_imgui.hh>
#include <limits>

static int cursor_xpos = 0;
static int cursor_ypos = 0;
static uint16_t buttons[GLFW_MOUSE_BUTTON_LAST + 1] = {};
static int scroll_dx = 0;
static int scroll_dy = 0;
static uint64_t element_id = 0U;
static uint64_t focused_id = UINT64_MAX;

static void on_cursor_move(const CursorMoveEvent &event)
{
    cursor_xpos = event.xpos;
    cursor_ypos = event.ypos;
}

static void on_mouse_button(const MouseButtonEvent &event)
{
    if(event.action == GLFW_PRESS) {
        buttons[event.button] = 1U;
    }
    else {
        buttons[event.button] = 0U;
        focused_id = UINT64_MAX;
    }
}

static void on_mouse_scroll(const MouseScrollEvent &event)
{
    scroll_dx += event.dx;
    scroll_dy += event.dy;
}

void ui::imgui::init()
{
    globals::dispatcher.sink<CursorMoveEvent>().connect<&on_cursor_move>();
    globals::dispatcher.sink<MouseButtonEvent>().connect<&on_mouse_button>();
    globals::dispatcher.sink<MouseScrollEvent>().connect<&on_mouse_scroll>();
}

void ui::imgui::update_late()
{
    for(size_t i = 0; i <= GLFW_MOUSE_BUTTON_LAST; ++i) {
        if(!buttons[i])
            continue;
        buttons[i] += 1U;
    }

    scroll_dx = 0;
    scroll_dy = 0;

    element_id = 0U;
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

    canvas::draw_text(sx, sy, text, font, style.text_shadow, style.text_background, fscale);
    canvas::draw_text(tx, ty, text, font, style.text_default, style.text_background, fscale);

    element_id += 1U;
}

bool ui::imgui::button(int xpos, int ypos, int width, const canvas::Text &text, const canvas::Font &font, const ui::Style &style)
{
    const int iscale = globals::ui_scale;

    const int px = iscale * style.rect_text_padding.x;
    const int py = iscale * style.rect_text_padding.y;

    const int tx = iscale * xpos + px;
    const int ty = iscale * ypos + py;
    const int th = iscale * font.get_glyph_height() * text.get_max_text_height();

    const int rx = iscale * xpos;
    const int ry = iscale * ypos;
    const int rw = iscale * width;
    const int rh = 2 * py + th;

    const int sx = tx + iscale;
    const int sy = ty + iscale;

    const int rxx = rx + rw;
    const int ryy = ry + rh;
    const bool intersect = (cursor_xpos >= rx && cursor_xpos < rxx && cursor_ypos >= ry && cursor_ypos < ryy);
    const bool hovered = (intersect && focused_id == UINT64_MAX);

    if(intersect && buttons[GLFW_MOUSE_BUTTON_LEFT] == 1U)
        focused_id = element_id;
    const bool focused = (element_id == focused_id);

    vector4d_t rect_col = {};
    vector4d_t text_col = {};

    if(focused) {
        rect_col = style.rect_pressed;
        text_col = style.text_pressed;
    }
    else if(hovered) {
        if(buttons[GLFW_MOUSE_BUTTON_LEFT]) {
            rect_col = style.rect_pressed;
            text_col = style.text_pressed;
        }
        else {
            rect_col = style.rect_hovered;
            text_col = style.text_hovered;
        }
    }
    else {
        rect_col = style.rect_default;
        text_col = style.text_default;
    }

    canvas::draw_rect(rx, ry, rw, rh, rect_col);
    canvas::draw_text(sx, sy, text, font, style.text_shadow, COL_TRANSPARENT, globals::ui_scale);
    canvas::draw_text(tx, ty, text, font, text_col, COL_TRANSPARENT, globals::ui_scale);

    element_id += 1U;

    if(focused || hovered)
        return buttons[GLFW_MOUSE_BUTTON_LEFT] == 1U;
    return false;
}

bool ui::imgui::slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style)
{
    return ui::imgui::slider(xpos, ypos, width, value, text, font, style, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), 0.0);
}

bool ui::imgui::slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style, double min, double max)
{
    return ui::imgui::slider(xpos, ypos, width, value, text, font, style, min, max, 0.0);
}

bool ui::imgui::slider(int xpos, int ypos, int width, double &value, const canvas::Text &text, const canvas::Font &font, const Style &style, double min, double max, double step)
{
    double new_value = value;

    const int iscale = globals::ui_scale;

    const int px = iscale * style.rect_text_padding.x;
    const int py = iscale * style.rect_text_padding.y;

    const int tx = iscale * xpos + px;
    const int ty = iscale * ypos + py;
    const int th = iscale * font.get_glyph_height() * text.get_max_text_height();

    const int rx = iscale * xpos;
    const int ry = iscale * ypos;
    const int rw = iscale * width;
    const int rh = 2 * py + th;

    const int sx = tx + iscale;
    const int sy = ty + iscale;

    const int lw = iscale * font.get_glyph_width();
    const int lx = rx + (rw - lw) * ((value - min) / (max - min));

    const int rxx = rx + rw;
    const int ryy = ry + rh;
    const bool intersect = (cursor_xpos >= rx && cursor_xpos < rxx && cursor_ypos >= ry && cursor_ypos < ryy);
    const bool hovered = (intersect && focused_id == UINT64_MAX);

    if(intersect && buttons[GLFW_MOUSE_BUTTON_LEFT] == 1U)
        focused_id = element_id;
    const bool focused = (element_id == focused_id);

    if(focused || (hovered && buttons[GLFW_MOUSE_BUTTON_LEFT])) {
        const auto lcur = static_cast<double>(cursor_xpos - rx - (lw / 2));
        const auto lmax = static_cast<double>(rw - lw);
        new_value = lcur / lmax * (max - min) + min;
    }
    else if(hovered && (scroll_dx || scroll_dy)) {
        const auto lstep = step ? step : 0.01;
        new_value += lstep * scroll_dx;
        new_value += lstep * scroll_dy;
    }

    if(step != 0.0)
        new_value = round(new_value / step) * step;
    new_value = cxmath::clamp(new_value, min, max);

    vector4d_t rect_col = {};
    vector4d_t text_col = {};
    vector4d_t line_col = {};

    if(focused) {
        rect_col = style.rect_pressed;
        text_col = style.text_pressed;
        line_col = style.slider_pressed;
    }
    else if(hovered) {
        if(buttons[GLFW_MOUSE_BUTTON_LEFT]) {
            rect_col = style.rect_pressed;
            text_col = style.text_pressed;
            line_col = style.slider_pressed;
        }
        else {
            rect_col = style.rect_hovered;
            text_col = style.text_hovered;
            line_col = style.slider_hovered;
        }
    }
    else {
        rect_col = style.rect_default;
        text_col = style.text_default;
        line_col = style.slider_default;
    }

    canvas::draw_rect(rx, ry, rw, rh, rect_col);
    canvas::draw_rect(lx, ry, lw, rh, line_col);
    canvas::draw_text(sx, sy, text, font, style.text_shadow, COL_TRANSPARENT, globals::ui_scale);
    canvas::draw_text(tx, ty, text, font, text_col, COL_TRANSPARENT, globals::ui_scale);

    element_id += 1;

    if(new_value != value) {
        value = new_value;
        return true;
    }

    return false;
}
