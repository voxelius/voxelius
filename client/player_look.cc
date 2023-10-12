// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/cursor_move.hh>
#include <client/globals.hh>
#include <client/options.hh>
#include <client/player_look.hh>
#include <shared/angle.hh>
#include <shared/entity/head.hh>

constexpr static const double PITCH_MIN = -1.0 * cxmath::radians(89.9);
constexpr static const double PITCH_MAX = +1.0 * cxmath::radians(89.9);

static double previous_cx = 0.0;
static double previous_cy = 0.0;

static void on_cursor_move(const CursorMoveEvent &event)
{
    if(!globals::ui_screen && globals::registry.valid(globals::player)) {
        const double sensitivity = options::controls::mouse_sensitivity;
        const double dx = event.xpos - previous_cx;
        const double dy = event.ypos - previous_cy;

        auto &head = globals::registry.get<HeadComponent>(globals::player);
        head.angles.x -= cxmath::radians(dy) * sensitivity;
        head.angles.y -= cxmath::radians(dx) * sensitivity;
        head.angles.x = cxmath::clamp(head.angles.x, PITCH_MIN, PITCH_MAX);
        head.angles = angle::wrap_180_n(head.angles);
    }

    previous_cx = event.xpos;
    previous_cy = event.ypos;
}

void player_look::init()
{
    previous_cx = globals::window_width / 2.0;
    previous_cy = globals::window_height / 2.0;
    globals::dispatcher.sink<CursorMoveEvent>().connect<&on_cursor_move>();
}
