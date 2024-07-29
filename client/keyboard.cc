// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug/debug_toggles.hh>
#include <client/event/glfw_key.hh>
#include <client/gui/settings.hh>
#include <client/keyboard.hh>
#include <client/entity/player_move.hh>
#include <client/globals.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/config.hh>

static int key_forward = GLFW_KEY_W;
static int key_back = GLFW_KEY_S;
static int key_left = GLFW_KEY_A;
static int key_right = GLFW_KEY_D;
static int key_down = GLFW_KEY_LEFT_SHIFT;
static int key_up = GLFW_KEY_SPACE;

// This direction vector is updated every time
// a key is pressed and then is submitted to player_move
static Vector3D keyboard_wishdir = {};

static void on_glfw_key(const GlfwKeyEvent &event)
{
    if(!globals::registry.valid(globals::player)) {
        // There's no point in updating movement
        // while we're not loaded into a world
        return;
    }

    if(debug_toggles::is_sequence_await || globals::gui_screen) {
        // Cannot move while awaiting for input
        // from other systems (either debug or GameUI)
        player_move::set_direction(Vector3D());
        return;
    }

    if(event.action == GLFW_PRESS) {
        if(event.key == key_forward) {
            keyboard_wishdir += Vector3D::dir_forward();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_back) {
            keyboard_wishdir += Vector3D::dir_back();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_left) {
            keyboard_wishdir += Vector3D::dir_left();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_right) {
            keyboard_wishdir += Vector3D::dir_right();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_down) {
            keyboard_wishdir += Vector3D::dir_down();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_up) {
            keyboard_wishdir += Vector3D::dir_up();
            player_move::set_direction(keyboard_wishdir);
            return;
        }
    }
    else if(event.action == GLFW_RELEASE) {
        if(event.key == key_forward) {
            keyboard_wishdir -= Vector3D::dir_forward();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_back) {
            keyboard_wishdir -= Vector3D::dir_back();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_left) {
            keyboard_wishdir -= Vector3D::dir_left();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_right) {
            keyboard_wishdir -= Vector3D::dir_right();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_down) {
            keyboard_wishdir -= Vector3D::dir_down();
            player_move::set_direction(keyboard_wishdir);
            return;
        }

        if(event.key == key_up) {
            keyboard_wishdir -= Vector3D::dir_up();
            player_move::set_direction(keyboard_wishdir);
            return;
        }
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

    settings::add_key_binding(1, settings::KEYBOARD_MOVEMENT, "key.forward", key_forward);
    settings::add_key_binding(2, settings::KEYBOARD_MOVEMENT, "key.back", key_back);
    settings::add_key_binding(3, settings::KEYBOARD_MOVEMENT, "key.left", key_left);
    settings::add_key_binding(4, settings::KEYBOARD_MOVEMENT, "key.right", key_right);
    settings::add_key_binding(5, settings::KEYBOARD_MOVEMENT, "key.down", key_down);
    settings::add_key_binding(6, settings::KEYBOARD_MOVEMENT, "key.up", key_up);

    globals::dispatcher.sink<GlfwKeyEvent>().connect<&on_glfw_key>();
}
