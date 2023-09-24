// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <client/input.hh>
#include <client/plook.hh>
#include <client/screen.hh>
#include <shared/cxmath.hh>
#include <shared/head.hh>
#include <shared/world.hh>

constexpr static const float PITCH_MIN = -1.0f * cxmath::radians(89.9f);
constexpr static const float PITCH_MAX = +1.0f * cxmath::radians(89.9f);

static float previous_cx = 0.0f;
static float previous_cy = 0.0f;

static void on_cursor_pos(const CursorPosEvent &event)
{
    if(globals::world.registry.valid(globals::player)) {
        const float dx = event.xpos - previous_cx;
        const float dy = event.ypos - previous_cy;

        auto &head = globals::world.registry.get<HeadComponent>(globals::player);
        head.euler.x -= dy;
        head.euler.y -= dx;
        head.euler.x = cxmath::clamp(head.euler.x, PITCH_MIN, PITCH_MAX);
    }

    previous_cx = event.xpos;
    previous_cy = event.ypos;
}

void plook::init()
{
    float width, height;
    screen::get_size(width, height);

    previous_cx = width / 2.0f;
    previous_cy = height / 2.0f;

    globals::dispatcher.sink<CursorPosEvent>().connect<&on_cursor_pos>();
}
