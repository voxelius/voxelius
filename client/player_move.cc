// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <array>
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/player_move.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glm/gtc/quaternion.hpp>
#include <shared/const.hh>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>

constexpr static const unsigned int IN_FW = 1U << 0U;
constexpr static const unsigned int IN_BK = 1U << 1U;
constexpr static const unsigned int IN_LF = 1U << 2U;
constexpr static const unsigned int IN_RT = 1U << 3U;
constexpr static const unsigned int IN_UP = 1U << 4U;
constexpr static const unsigned int IN_DN = 1U << 5U;

static unsigned int in_keys = 0U;

static glm::dvec2 accelerate_2D(const glm::dvec2 &velocity, const glm::dvec2 &direction, double wish_speed, double accel)
{
    const double current_speed = glm::dot(velocity, direction);
    const double add_speed = wish_speed - current_speed;

    if(add_speed <= 0.0)
        return velocity;
    return velocity + direction * cxmath::min(accel * wish_speed * globals::frametime, add_speed);
}

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.action == GLFW_PRESS) {
        switch(event.key) {
            case GLFW_KEY_W:
            case GLFW_KEY_UP:
                in_keys |= IN_FW;
                break;
            case GLFW_KEY_S:
            case GLFW_KEY_DOWN:
                in_keys |= IN_BK;
                break;
            case GLFW_KEY_A:
            case GLFW_KEY_LEFT:
                in_keys |= IN_LF;
                break;
            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT:
                in_keys |= IN_RT;
                break;
            case GLFW_KEY_SPACE:
                in_keys |= IN_UP;
                break;
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT:
                in_keys |= IN_DN;
                break;
        }
    }
    else if(event.action == GLFW_RELEASE) {
        switch(event.key) {
            case GLFW_KEY_W:
            case GLFW_KEY_UP:
                in_keys &= ~IN_FW;
                break;
            case GLFW_KEY_S:
            case GLFW_KEY_DOWN:
                in_keys &= ~IN_BK;
                break;
            case GLFW_KEY_A:
            case GLFW_KEY_LEFT:
                in_keys &= ~IN_LF;
                break;
            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT:
                in_keys &= ~IN_RT;
                break;
            case GLFW_KEY_SPACE:
                in_keys &= ~IN_UP;
                break;
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT:
                in_keys &= ~IN_DN;
                break;
        }
    }
}

void player_move::init()
{
    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void player_move::update()
{
    if(globals::registry.valid(globals::player)) {
        const auto &head = globals::registry.get<HeadComponent>(globals::player);
        auto &transform = globals::registry.get<TransformComponent>(globals::player);
        auto &velocity = globals::registry.get<VelocityComponent>(globals::player);

        glm::dvec3 direction = {};
        if(!globals::ui_screen) {
            if(in_keys & IN_FW)
                direction += DIR_FORWARD;
            if(in_keys & IN_BK)
                direction -= DIR_FORWARD;
            if(in_keys & IN_LF)
                direction -= DIR_RIGHT;
            if(in_keys & IN_RT)
                direction += DIR_RIGHT;
            if(in_keys & IN_UP)
                direction += DIR_UP;
            if(in_keys & IN_DN)
                direction -= DIR_UP;
        }

        const double speed = 16.0;
        const double accelerate = 0.05;
        const double decelerate = 0.15;

        if(direction.x || direction.z) {
            const auto move = glm::dvec3{direction.x, 0.0, direction.z};
            const auto vel = glm::dquat{{0.0, head.angles.y, 0.0}} * move * speed;
            velocity.linear.x = glm::mix(velocity.linear.x, vel.x, accelerate);
            velocity.linear.z = glm::mix(velocity.linear.z, vel.z, accelerate);
        }
        else {
            velocity.linear.x = glm::mix(velocity.linear.x, 0.0, decelerate);
            velocity.linear.z = glm::mix(velocity.linear.z, 0.0, decelerate);
        }

        if(direction.y) {
            velocity.linear.y = glm::mix(velocity.linear.y, direction.y * speed, accelerate);
        }
        else {
            velocity.linear.y = glm::mix(velocity.linear.y, 0.0, decelerate);
        }
    }
}
