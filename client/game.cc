// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/game.hh>
#include <client/globals.hh>
#include <client/input.hh>
#include <client/player_look.hh>
#include <client/player_move.hh>
#include <client/screen.hh>
#include <shared/head.hh>
#include <shared/inertial.hh>
#include <shared/player.hh>
#include <shared/transform.hh>
#include <shared/vdef.hh>
#include <shared/velocity.hh>
#include <spdlog/spdlog.h>

static void on_key(const KeyEvent &event)
{
    if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
        spdlog::info("GLFW_KEY_ESCAPE has been pressed! Exiting...");
        glfwSetWindowShouldClose(globals::window, true);
    }
}

static void on_screen_size(const ScreenSizeEvent &event)
{
    spdlog::trace("{}x{} [{}]", event.width, event.height, event.aspect);
}

void client_game::init()
{
    input::init();
    screen::init();

    player_look::init();
    player_move::init();

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
    globals::dispatcher.sink<ScreenSizeEvent>().connect<&on_screen_size>();
}

void client_game::init_late()
{
    screen::init_late();

    vdef::assign("stone", 0x0001);

    spdlog::info("spawning local player");
    globals::player = globals::world.registry.create();
    globals::world.registry.emplace<PlayerComponent>(globals::player);
    globals::world.registry.emplace<HeadComponent>(globals::player);
    globals::world.registry.emplace<TransformComponent>(globals::player);
    globals::world.registry.emplace<VelocityComponent>(globals::player);

    glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void client_game::deinit()
{

}

void client_game::update()
{
    player_move::update();

    inertial::update(globals::world, globals::frametime);
}

void client_game::update_late()
{
    VelocityComponent &vel = globals::world.registry.get<VelocityComponent>(globals::player);
    spdlog::info("VEL: {} {} {} {}", vel.velocity.x, vel.velocity.y, vel.velocity.z, glm::length(vel.velocity));
}

void client_game::render()
{
    
}
