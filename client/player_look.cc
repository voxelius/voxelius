// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <client/event/glfw_cursor_pos.hh>
#include <client/globals.hh>
#include <client/player_look.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/angle.hh>
#include <shared/config.hh>
#include <shared/entity/head.hh>

constexpr static const double PITCH_MIN = -1.0 * glm::radians(89.9);
constexpr static const double PITCH_MAX = +1.0 * glm::radians(89.9);

static double previous_cx = 0.0;
static double previous_cy = 0.0;

bool player_look::raw_input = true;
double player_look::sensitivity = 0.25;

static void on_glfw_cursor_pos(const GlfwCursorPosEvent &event)
{
    if(!globals::ui_screen && globals::registry.valid(globals::player)) {
        const double dx = event.xpos - previous_cx;
        const double dy = event.ypos - previous_cy;

        auto &head = globals::registry.get<HeadComponent>(globals::player);
        head.angles.x -= glm::radians(dy) * player_look::sensitivity;
        head.angles.y -= glm::radians(dx) * player_look::sensitivity;
        head.angles.x = glm::clamp(head.angles.x, PITCH_MIN, PITCH_MAX);
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
