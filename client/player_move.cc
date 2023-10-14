// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <bitset>
#include <client/event/keyboard_key.hh>
#include <client/globals.hh>
#include <client/player_move.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glm/gtc/quaternion.hpp>
#include <shared/const.hh>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>

constexpr static const size_t MOVE_FD = 0;
constexpr static const size_t MOVE_BK = 1;
constexpr static const size_t MOVE_LF = 2;
constexpr static const size_t MOVE_RT = 3;
constexpr static const size_t MOVE_UP = 4;
constexpr static const size_t MOVE_DN = 5;

static std::bitset<8> move_keys = {};

static void on_keyboard_key(const KeyboardKeyEvent &event)
{
    const bool act_dn = {event.action == GLFW_PRESS};
    const bool act_up = {event.action == GLFW_RELEASE};

    if(act_dn || act_up) {
        switch(event.key) {
            case GLFW_KEY_W:
            case GLFW_KEY_UP:
                move_keys.set(MOVE_FD, act_dn);
                break;
            case GLFW_KEY_S:
            case GLFW_KEY_DOWN:
                move_keys.set(MOVE_BK, act_dn);
                break;
            case GLFW_KEY_A:
            case GLFW_KEY_LEFT:
                move_keys.set(MOVE_LF, act_dn);
                break;
            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT:
                move_keys.set(MOVE_RT, act_dn);
                break;
            case GLFW_KEY_SPACE:
                move_keys.set(MOVE_UP, act_dn);
                break;
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT:
                move_keys.set(MOVE_DN, act_dn);
                break;
        }
    }
}

void player_move::init()
{
    globals::dispatcher.sink<KeyboardKeyEvent>().connect<&on_keyboard_key>();
}

void player_move::update()
{
    if(globals::registry.valid(globals::player)) {
        glm::dvec3 direction = {0.0, 0.0, 0.0};
        if(!globals::ui_screen) {
            if(move_keys[MOVE_FD])
                direction += DIR_FORWARD;
            if(move_keys[MOVE_BK])
                direction -= DIR_FORWARD;
            if(move_keys[MOVE_LF])
                direction -= DIR_RIGHT;
            if(move_keys[MOVE_RT])
                direction += DIR_RIGHT;
            if(move_keys[MOVE_UP])
                direction += DIR_UP;
            if(move_keys[MOVE_DN])
                direction -= DIR_UP;
        }

        const auto &head = globals::registry.get<HeadComponent>(globals::player);
        auto &velocity = globals::registry.get<VelocityComponent>(globals::player);
        velocity.linear = glm::dquat{glm::dvec3{0.0, head.angles.y, 0.0}} * direction * 16.0;
    }
}
