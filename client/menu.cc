// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/menu.hh>
#include <client/ui/label.hh>
#include <client/ui/rect.hh>

static ui::Font title_font = {};
static ui::Label title_label = {};

void menu::init()
{
    title_font.load_rom("/fonts/IBM_VGA_8x16.bin", 8, 16);
    title_label.set_text(L"Voxelius");
    title_label.set_center({0, 96});
    title_label.set_scale({3.0, 3.0});
}

void menu::deinit()
{
    title_label.destroy();
    title_font.unload();
}

void menu::draw_ui()
{
    ui::Rect rect = {};
    rect.set_color({1.0, 0.0, 0.0, 1.0});
    rect.set_size({640, 480});
    ui::Rect::draw(rect);

    ui::Label::draw(title_label, title_font);
}

