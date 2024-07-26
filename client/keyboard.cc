// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/globals.hh>
#include <client/keyboard.hh>
#include <client/settings.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <glm/gtc/quaternion.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/config.hh>
#include <shared/const.hh>

constexpr static float PLAYER_MOVE_SPEED = 16.0f;
constexpr static float PLAYER_ACCELERATE = 0.05f;
constexpr static float PLAYER_DECELERATE = 0.15f;

using KeyboardBits = unsigned short;
constexpr static KeyboardBits KB_NULL       = 0x0000;
constexpr static KeyboardBits KB_FORWARD    = 0x0001;
constexpr static KeyboardBits KB_BACK       = 0x0002;
constexpr static KeyboardBits KB_LEFT       = 0x0004;
constexpr static KeyboardBits KB_RIGHT      = 0x0008;
constexpr static KeyboardBits KB_UP         = 0x0010;
constexpr static KeyboardBits KB_DOWN       = 0x0020;

static int key_forward  = GLFW_KEY_W;
static int key_back     = GLFW_KEY_S;
static int key_left     = GLFW_KEY_A;
static int key_right    = GLFW_KEY_D;
static int key_up       = GLFW_KEY_SPACE;
static int key_down     = GLFW_KEY_LEFT_SHIFT;

static KeyboardBits pressed_keys = KB_NULL;

static inline void update_key(KeyboardBits key_bit, int action)
{
    if(action == GLFW_PRESS) {
        pressed_keys |= key_bit;
        return;
    }

    if(action == GLFW_RELEASE) {
        pressed_keys &= ~key_bit;
        return;
    }
}

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.key == key_forward) {
        update_key(KB_FORWARD, event.action);
        return;
    }

    if(event.key == key_back) {
        update_key(KB_BACK, event.action);
        return;
    }

    if(event.key == key_left) {
        update_key(KB_LEFT, event.action);
        return;
    }

    if(event.key == key_right) {
        update_key(KB_RIGHT, event.action);
        return;
    }

    if(event.key == key_up) {
        update_key(KB_UP, event.action);
        return;
    }

    if(event.key == key_down) {
        update_key(KB_DOWN, event.action);
        return;
    }
}

void keyboard::init(void)
{
    Config::add(globals::client_config, "key.forward", key_forward);
    Config::add(globals::client_config, "key.back", key_back);
    Config::add(globals::client_config, "key.left", key_left);
    Config::add(globals::client_config, "key.right", key_right);
    Config::add(globals::client_config, "key.up", key_up);
    Config::add(globals::client_config, "key.down", key_down);

    settings::add_key_binding(1, settings::KEYBOARD_MOVEMENT, "key.forward", key_forward);
    settings::add_key_binding(2, settings::KEYBOARD_MOVEMENT, "key.back", key_back);
    settings::add_key_binding(3, settings::KEYBOARD_MOVEMENT, "key.left", key_left);
    settings::add_key_binding(4, settings::KEYBOARD_MOVEMENT, "key.right", key_right);
    settings::add_key_binding(5, settings::KEYBOARD_MOVEMENT, "key.up", key_up);
    settings::add_key_binding(6, settings::KEYBOARD_MOVEMENT, "key.down", key_down);

    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}

void keyboard::update(void)
{
    if(!globals::registry.valid(globals::player)) {
        // There's no point in updating movement
        // while we're not loaded into a world
        return;
    }

    glm::fvec3 direction = {};

    if(!globals::ui_screen) {
        if(pressed_keys & KB_FORWARD)
            direction += DIR_FORWARD;
        if(pressed_keys & KB_BACK)
            direction += DIR_BACK;
        if(pressed_keys & KB_LEFT)
            direction += DIR_LEFT;
        if(pressed_keys & KB_RIGHT)
            direction += DIR_RIGHT;
        if(pressed_keys & KB_UP)
            direction += DIR_UP;
        if(pressed_keys & KB_DOWN)
            direction += DIR_DOWN;
    }

    const auto &head = globals::registry.get<HeadComponent>(globals::player);
    auto &transform = globals::registry.get<TransformComponent>(globals::player);
    auto &velocity = globals::registry.get<VelocityComponent>(globals::player);

    if(direction.x || direction.z) {
        const auto orient = glm::fquat(glm::fvec3(0.0f, head.angles.y, 0.0f));
        const auto wishdir = glm::fvec3(direction.x, 0.0f, direction.z);
        const auto wishvel = orient * wishdir * PLAYER_MOVE_SPEED;
        velocity.linear.x = glm::mix(velocity.linear.x, wishvel.x, PLAYER_ACCELERATE);
        velocity.linear.z = glm::mix(velocity.linear.z, wishvel.z, PLAYER_ACCELERATE);
    }
    else {
        velocity.linear.x = glm::mix(velocity.linear.x, 0.0f, PLAYER_DECELERATE);
        velocity.linear.z = glm::mix(velocity.linear.z, 0.0f, PLAYER_DECELERATE);
    }

    // FIXME: this only does flight move now; maybe
    // add some collision detection in the future you lazy fuck?

    if(direction.y)
        velocity.linear.y = glm::mix(velocity.linear.y, direction.y * PLAYER_MOVE_SPEED, PLAYER_ACCELERATE);
    else velocity.linear.y = glm::mix(velocity.linear.y, 0.0f, PLAYER_DECELERATE);
}
