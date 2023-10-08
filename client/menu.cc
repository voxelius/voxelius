// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/menu.hh>
#include <client/screen.hh>
#include <client/ui_color.hh>
#include <client/ui_draw.hh>
#include <client/ui_font.hh>
#include <client/ui_immediate.hh>
#include <client/ui_text.hh>
#include <spdlog/spdlog.h>

static ui::Text title = {};
static ui::Text button = {};
static ui::im::Style style = {};

void menu::init()
{
    title.create(32, 1);
    title.set(0, L"Voxelius");

    button.create(64, 1);

    style.element_scale = 2U;
    style.button_background[ui::im::BUTTON_IDLE] = ui::TRANSPARENT;
    style.button_background[ui::im::BUTTON_HOVER] = {1.0, 1.0, 1.0, 0.125};
    style.button_background[ui::im::BUTTON_PRESS] = {1.0, 1.0, 1.0, 0.250};
    style.button_foreground[ui::im::BUTTON_IDLE] = ui::LIGHT_GRAY;
    style.button_foreground[ui::im::BUTTON_HOVER] = ui::WHITE;
    style.button_foreground[ui::im::BUTTON_PRESS] = ui::WHITE;
    style.button_margin = vector2i_t{4, 4};
}

void menu::deinit()
{
    button.destroy();
    title.destroy();
}

void menu::draw()
{
    int xpos = 16;
    int ypos = 16;
    int tstep = globals::pc_vga_8x16.get_glyph_height() * 2U + 16;
    int bstep = (2 * style.button_margin.y + globals::pc_vga_8x8.get_glyph_height());

    ui::draw::text(4 + xpos / 2, ypos / 2, title, globals::pc_vga_8x16, ui::WHITE, 2U * style.element_scale);
    ypos += tstep;

    button.set(0, L"DEBUG SESSION");
    if(ui::im::button(xpos, ypos, button, globals::pc_vga_8x8, style)) {
        spdlog::info("DEBUG SESSION");
    }
    ypos += bstep;

    button.set(0, L"Join a Server");
    if(ui::im::button(xpos, ypos, button, globals::pc_vga_8x8, style)) {
        spdlog::info("JOIN A SERVER");
    }
    ypos += bstep;

    button.set(0, L"Settings");
    if(ui::im::button(xpos, ypos, button, globals::pc_vga_8x8, style)) {
        spdlog::info("SETTINGS");
    }
    ypos += bstep;

    button.set(0, L"Quit");
    if(ui::im::button(xpos, ypos, button, globals::pc_vga_8x8, style)) {
        spdlog::info("QUIT?");
    }
    ypos += bstep;
}
