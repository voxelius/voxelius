// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <bitset>
#include <client/globals.hh>
#include <client/input.hh>
#include <client/pmove.hh>
#include <shared/const.hh>
#include <shared/head.hh>
#include <shared/transform.hh>
#include <shared/velocity.hh>
#include <shared/world.hh>

constexpr static const size_t MOVE_FD = 0;
constexpr static const size_t MOVE_BK = 1;
constexpr static const size_t MOVE_LF = 2;
constexpr static const size_t MOVE_RT = 3;
constexpr static const size_t MOVE_UP = 4;
constexpr static const size_t MOVE_DN = 5;

static bool needs_update = false;
static std::bitset<8> move_keys = {};

static void on_key(const KeyEvent &event)
{
    const bool act_dn = {event.action == GLFW_PRESS};
    const bool act_up = {event.action == GLFW_RELEASE};

    if(act_dn || act_up) {
        switch(event.key) {
            case GLFW_KEY_W:
            case GLFW_KEY_UP:
                needs_update = true;
                move_keys.set(MOVE_FD, act_dn);
                break;
            case GLFW_KEY_S:
            case GLFW_KEY_DOWN:
                needs_update = true;
                move_keys.set(MOVE_BK, act_dn);
                break;
            case GLFW_KEY_A:
            case GLFW_KEY_LEFT:
                needs_update = true;
                move_keys.set(MOVE_LF, act_dn);
                break;
            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT:
                needs_update = true;
                move_keys.set(MOVE_RT, act_dn);
                break;
            case GLFW_KEY_SPACE:
                needs_update = true;
                move_keys.set(MOVE_UP, act_dn);
                break;
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT:
                needs_update = true;
                move_keys.set(MOVE_DN, act_dn);
                break;
        }
    }
}

void pmove::init()
{
    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}

void pmove::update()
{
    if(globals::world.registry.valid(globals::player) && needs_update) {
        vec3f_t direction = {};

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

        const auto &head = globals::world.registry.get<HeadComponent>(globals::player);
        const auto &transform = globals::world.registry.get<TransformComponent>(globals::player);

        auto &velocity = globals::world.registry.get<VelocityComponent>(globals::player);
        velocity.velocity = transform.rotation * quatf_t{head.euler} * direction * 5.0f;

        needs_update = false;
    }
}
