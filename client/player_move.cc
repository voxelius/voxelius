// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <array>
#include <client/event/key.hh>
#include <client/globals.hh>
#include <client/player_move.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glm/gtc/quaternion.hpp>
#include <shared/const.hh>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>

constexpr static const size_t MOVE_FT = 0;
constexpr static const size_t MOVE_BK = 1;
constexpr static const size_t MOVE_LF = 2;
constexpr static const size_t MOVE_RT = 3;
constexpr static const size_t MOVE_UP = 4;
constexpr static const size_t MOVE_DN = 5;

static unsigned int move_keys[8] = {};

static void on_key(const KeyEvent &event)
{
    if(event.action == GLFW_PRESS) {
        switch(event.key) {
            case GLFW_KEY_W:
            case GLFW_KEY_UP:
                move_keys[MOVE_FT] = 1U;
                break;
            case GLFW_KEY_S:
            case GLFW_KEY_DOWN:
                move_keys[MOVE_BK] = 1U;
                break;
            case GLFW_KEY_A:
            case GLFW_KEY_LEFT:
                move_keys[MOVE_LF] = 1U;
                break;
            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT:
                move_keys[MOVE_RT] = 1U;
                break;
            case GLFW_KEY_SPACE:
                move_keys[MOVE_UP] = 1U;
                break;
        }
    }
    else if(event.action == GLFW_RELEASE) {
        switch(event.key) {
            case GLFW_KEY_W:
            case GLFW_KEY_UP:
                move_keys[MOVE_FT] = 0U;
                break;
            case GLFW_KEY_S:
            case GLFW_KEY_DOWN:
                move_keys[MOVE_BK] = 0U;
                break;
            case GLFW_KEY_A:
            case GLFW_KEY_LEFT:
                move_keys[MOVE_LF] = 0U;
                break;
            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT:
                move_keys[MOVE_RT] = 0U;
                break;
            case GLFW_KEY_SPACE:
                move_keys[MOVE_UP] = 0U;
                break;
        }
    }
}

void player_move::init()
{
    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}

void player_move::update()
{
    if(globals::registry.valid(globals::player)) {
        glm::dvec3 direction = {};

        if(!globals::ui_screen) {
            if(move_keys[MOVE_FT])
                direction += DIR_FORWARD;
            if(move_keys[MOVE_BK])
                direction -= DIR_FORWARD;
            if(move_keys[MOVE_LF])
                direction -= DIR_RIGHT;
            if(move_keys[MOVE_RT])
                direction += DIR_RIGHT;
            if(move_keys[MOVE_UP] == 1U) // Just one frame
                direction += DIR_UP * 4.0;
        }

        const auto &head = globals::registry.get<HeadComponent>(globals::player);
        auto &velocity = globals::registry.get<VelocityComponent>(globals::player);
        const auto want = glm::dquat{glm::dvec3{0.0, head.angles.y, 0.0}} * direction * 3.0;
        velocity.linear.x = want.x;
        velocity.linear.z = want.z;
        velocity.linear.y += want.y;
        velocity.linear.y -= 0.8;

        for(unsigned int k = 0U; k < 8U; ++k) {
            if(!move_keys[k])
                continue;
            move_keys[k] += 1U;
        }
    }
}
