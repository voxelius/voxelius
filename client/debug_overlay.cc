// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/debug_overlay.hh>
#include <client/globals.hh>
#include <client/ui_text.hh>
#include <spdlog/fmt/fmt.h>

static ui::Text text = {};

void debug_overlay::init()
{
    text.create(64, 8);
    text.set_position({4, 4});
    text.set_background({0.0, 0.0, 0.0, 0.5});
    text.set_foreground({1.0, 1.0, 1.0, 1.0});

    // Print game version information
    // UNDONE: configure_file in cmake and pull
    // release info from it instead of hardcoding things
    const char *version = "0.0.1";
    const char *git_rev = "00000000";
    text.set_text(0, fmt::format("Voxelius {} [{}]", version, git_rev));

    // Rendering information
    const char *gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    const char *gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    text.set_text(1, fmt::format("OpenGL: {}", gl_version));
    text.set_text(2, fmt::format("Renderer: {}", gl_renderer));
}

void debug_overlay::deinit()
{
    text.destroy();
}

void debug_overlay::render2D()
{
    // Performance information
    const double fps = 1.0 / globals::frametime_avg;
    const double ms = 1000.0 * globals::frametime_avg;
    text.set_text(3, fmt::format("{:.0f} FPS // {:.02f} ms", fps, ms));
    ui::Text::draw(text, globals::pc_vga_8x16);
}
