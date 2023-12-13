// SPDX-License-Identifier: GPL-2.0-only
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas.hh>
#include <client/canvas_font.hh>
#include <client/canvas_text.hh>
#include <client/debug_overlay.hh>
#include <client/event/key.hh>
#include <client/globals.hh>
#include <client/voxel_mesher.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/config/config.hh>
#include <shared/config/number.hh>
#include <shared/coord.hh>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/version.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/xchar.h>

// Using a config variable makes debug
// overlay verbosity persist between restarts
static config::Number<unsigned int> overlay_level = 0U;

static canvas::Text text = {};
static double decim_frametime = 0.0;
static uint64_t decim_timer = 0;

static void on_key(const KeyEvent &event)
{
    if(event.action == GLFW_PRESS && event.key == GLFW_KEY_F3) {
        overlay_level.value += 1U;
        overlay_level.value %= 3U;
    }
}

void debug_overlay::init()
{
    config::add("debug.overlay_level", overlay_level);

    text.create(128, 1);
    decim_frametime = 0.0;
    decim_timer = 0;

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}

void debug_overlay::deinit()
{
    text.destroy();
}

void debug_overlay::render()
{
    const auto scale = cxmath::max(1U, globals::ui_scale / 2U);
    const auto ystep = globals::font_16px.get_glyph_height() * scale;
    const auto background = glm::dvec4{0.0, 0.0, 0.0, 0.50};
    const auto foreground = COLOR_WHITE;

    int ypos = 8;

    text.set(0, L"Voxelius Indev " VERSION_STRING_W);
    canvas::draw_text(8, ypos, text, globals::font_16px, foreground, background, scale);
    ypos += ystep;

    if(overlay_level.value >= 2U) {
        text.set(0, CMAKE_CXX_COMPILER_ID_W L" C++ " CMAKE_CXX_COMPILER_VERSION_W L" " CMAKE_SYSTEM_NAME_W L"/" CMAKE_SYSTEM_PROCESSOR_W);
        canvas::draw_text(8, ypos, text, globals::font_16px, foreground, background, scale);
        ypos += ystep;
    }

    ypos += ystep / 4;

    if(overlay_level.value >= 1U) {
        if(globals::curtime >= decim_timer) {
            decim_frametime = globals::frametime_avg;
            decim_timer = globals::curtime + 60000;
        }

        text.set(0, fmt::format(L"{:.02f} FPS ({:.03f} ms)", 1.0 / decim_frametime, 1000.0 * decim_frametime));
        canvas::draw_text(8, ypos, text, globals::font_16px, foreground,background, scale);
        ypos += ystep + ystep / 4;

        if(globals::registry.valid(globals::player)) {
            const auto &head = globals::registry.get<HeadComponent>(globals::player);
            const auto &transform = globals::registry.get<TransformComponent>(globals::player);

            text.set(0, fmt::format(L"Position: [{:.03f} {:.03f} {:.03f}]", transform.position.x, transform.position.y, transform.position.z));
            canvas::draw_text(8, ypos, text, globals::font_16px, foreground,background, scale);
            ypos += ystep;

            text.set(0, fmt::format(L"Head: [{:.02f} {:.02f}]", cxmath::degrees(head.angles.x), cxmath::degrees(head.angles.y)));
            canvas::draw_text(8, ypos, text, globals::font_16px, foreground,background, scale);
            ypos += ystep;

            if(overlay_level.value >= 2U) {
                const auto vpos = coord::to_voxel(transform.position);
                const auto cpos = coord::to_chunk(vpos);

                text.set(0, fmt::format("Chunk: [{} {} {}] [{} {} {}]", cpos.x, cpos.y, cpos.z, vpos.x, vpos.y, vpos.z));
                canvas::draw_text(8, ypos, text, globals::font_16px, foreground,background, scale);
                ypos += ystep;
            }

            ypos += ystep / 4;
        }
    }

    if(overlay_level.value >= 2U) {
        text.set(0, fmt::format("Vertices drawn: {}", globals::vertices_drawn));
        canvas::draw_text(8, ypos, text, globals::font_16px, foreground,background, scale);
        ypos += ystep;

        text.set(0, fmt::format(L"Mesher queue size: {}", voxel_mesher::get_queue_size()));
        canvas::draw_text(8, ypos, text, globals::font_16px, foreground,background, scale);
        ypos += ystep;
    }
}
