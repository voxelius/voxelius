// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <client/event/glfw_cursor_pos.hh>
#include <client/globals.hh>
#include <client/player_look.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/angle.hh>
#include <shared/config/boolean.hh>
#include <shared/config/config.hh>
#include <shared/config/number.hh>
#include <shared/entity/head.hh>

constexpr static const double PITCH_MIN = -1.0 * cxmath::radians(89.9);
constexpr static const double PITCH_MAX = +1.0 * cxmath::radians(89.9);

static double previous_cx = 0.0;
static double previous_cy = 0.0;

config::Boolean player_look::raw_input = config::Boolean{true};
config::Number<double> player_look::sensitivity = config::Number<double>{0.25};

static void on_glfw_cursor_pos(const GlfwCursorPosEvent &event)
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
    config::add("player_look.raw_input", player_look::raw_input);
    config::add("player_look.sensitivity", player_look::sensitivity);

    previous_cx = globals::width / 2;
    previous_cy = globals::height / 2;

    globals::dispatcher.sink<GlfwCursorPosEvent>().connect<&on_glfw_cursor_pos>();
}
