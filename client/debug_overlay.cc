// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/debug_overlay.hh>
#include <client/globals.hh>
#include <client/ui/label.hh>
#include <spdlog/fmt/fmt.h>

static ui::Label label = {};

void debug_overlay::init()
{
    label.create(64, 8);
    label.set_position({4, 4});
    label.set_background({0.0, 0.0, 0.0, 0.5});
    label.set_foreground({1.0, 1.0, 1.0, 1.0});
    label.set_text(0, fmt::format("GL_VERSION: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION))));
    label.set_text(1, fmt::format("GL_RENDERER: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER))));
    label.set_text(3, "hui sosi guboy tryasi");
}

void debug_overlay::deinit()
{
    label.destroy();
}

void debug_overlay::render2D()
{
    label.set_text(2, fmt::format("{:.0f} FPS / {:.02f} ms", 1.0 / globals::frametime_avg, 1000.0 * globals::frametime_avg));
    ui::Label::draw(label, globals::pc_vga_8x16);
}

