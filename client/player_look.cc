// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/cursor_move.hh>
#include <client/globals.hh>
#include <client/player_look.hh>
#include <shared/angle.hh>
#include <shared/config/config.hh>
#include <shared/entity/head.hh>

constexpr static const double PITCH_MIN = -1.0 * cxmath::radians(89.9);
constexpr static const double PITCH_MAX = +1.0 * cxmath::radians(89.9);

static double previous_cx = 0.0;
static double previous_cy = 0.0;

config::Boolean player_look::raw_input = config::Boolean{true};
config::Double player_look::sensitivity = config::Double{0.25};

static void on_cursor_move(const CursorMoveEvent &event)
{
    if(!globals::ui_screen && globals::registry.valid(globals::player)) {
        const double dx = event.xpos - previous_cx;
        const double dy = event.ypos - previous_cy;

        auto &head = globals::registry.get<HeadComponent>(globals::player);
        head.angles.x -= cxmath::radians(dy) * player_look::sensitivity.value;
        head.angles.y -= cxmath::radians(dx) * player_look::sensitivity.value;
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
    config::add("player_look.raw_input", player_look::raw_input);
    config::add("player_look.sensitivity", player_look::sensitivity);
    globals::dispatcher.sink<CursorMoveEvent>().connect<&on_cursor_move>();
}
