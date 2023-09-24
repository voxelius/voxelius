// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/game.hh>
#include <client/globals.hh>
#include <client/input.hh>
#include <client/plook.hh>
#include <client/pmove.hh>
#include <client/screen.hh>
#include <client/shaders.hh>
#include <client/view.hh>
#include <client/voxel_mesher.hh>
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
    shaders::init();

    plook::init();
    pmove::init();

    voxel_mesher::init();

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
    globals::dispatcher.sink<ScreenSizeEvent>().connect<&on_screen_size>();
}

void client_game::init_late()
{
    screen::init_late();

    vdef::assign(Identifier{"voxelius:stone"}, 0x0001);

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
    voxel_mesher::deinit();
}

void client_game::update()
{
    pmove::update();

    view::update();

    voxel_mesher::update();

    inertial::update(globals::world, globals::frametime);
}

void client_game::update_late()
{

}

void client_game::render()
{

}
