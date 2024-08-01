// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_key.hh>
#include <client/input/keyboard.hh>
#include <client/entity/player_move.hh>
#include <client/globals.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/config.hh>

constexpr static unsigned int KEY_BIT_NULL      = 0x0000U;
constexpr static unsigned int KEY_BIT_FORWARD   = 0x0001U;
constexpr static unsigned int KEY_BIT_BACK      = 0x0002U;
constexpr static unsigned int KEY_BIT_LEFT      = 0x0004U;
constexpr static unsigned int KEY_BIT_RIGHT     = 0x0008U;
constexpr static unsigned int KEY_BIT_DOWN      = 0x0010U;
constexpr static unsigned int KEY_BIT_UP        = 0x0020U;

static int key_forward  = GLFW_KEY_W;
static int key_back     = GLFW_KEY_S;
static int key_left     = GLFW_KEY_A;
static int key_right    = GLFW_KEY_D;
static int key_down     = GLFW_KEY_LEFT_SHIFT;
static int key_up       = GLFW_KEY_SPACE;

// The keyboard subsytems sends updated direction
// values to player_move only when a key event is fired.
// As it turns out, GLFW can and does multiple GLFW_PRESS
// events for the same key in a row, so to prevent getting
// stuck moving in one direction, the direction vector is no
// longer kept as-is and instead is replaced with a bit field.
static unsigned int pressed_key_bits = KEY_BIT_NULL;

static void update_key_bits(unsigned int key_bit, int action)
{
    if(action == GLFW_PRESS) {
        // The key is pressed. If there was
        // any stuck bit it's now stuck for a reason
        pressed_key_bits |= key_bit;
    }
    else if(action == GLFW_RELEASE) {
        // The key event can also have GLFW_REPEAT
        // set as action so we only consider a key
        // to be released when we're explicitly told to
        pressed_key_bits &= ~key_bit;
    }
    
    Vec3f new_wishdir = Vec3f(0.0f, 0.0f, 0.0f);
    
    if(pressed_key_bits & KEY_BIT_FORWARD)
        new_wishdir += Vec3f::dir_forward();

    if(pressed_key_bits & KEY_BIT_BACK)
        new_wishdir += Vec3f::dir_back();
    
    if(pressed_key_bits & KEY_BIT_LEFT)
        new_wishdir += Vec3f::dir_left();

    if(pressed_key_bits & KEY_BIT_RIGHT)
        new_wishdir += Vec3f::dir_right();
    
    if(pressed_key_bits & KEY_BIT_DOWN)
        new_wishdir += Vec3f::dir_down();
    
    if(pressed_key_bits & KEY_BIT_UP)
        new_wishdir += Vec3f::dir_up();
    
    player_move::set_direction(new_wishdir);
}

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(event.key == key_forward) {
        update_key_bits(KEY_BIT_FORWARD, event.action);
        return;
    }

    if(event.key == key_back) {
        update_key_bits(KEY_BIT_BACK, event.action);
        return;
    }

    if(event.key == key_left) {
        update_key_bits(KEY_BIT_LEFT, event.action);
        return;
    }

    if(event.key == key_right) {
        update_key_bits(KEY_BIT_RIGHT, event.action);
        return;
    }

    if(event.key == key_down) {
        update_key_bits(KEY_BIT_DOWN, event.action);
        return;
    }

    if(event.key == key_up) {
        update_key_bits(KEY_BIT_UP, event.action);
        return;
    }
}

void keyboard::init(void)
{
    Config::add(globals::client_config, "key.forward", key_forward);
    Config::add(globals::client_config, "key.back", key_back);
    Config::add(globals::client_config, "key.left", key_left);
    Config::add(globals::client_config, "key.right", key_right);
    Config::add(globals::client_config, "key.down", key_down);
    Config::add(globals::client_config, "key.up", key_up);

    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}
