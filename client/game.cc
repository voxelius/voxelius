// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/game.hh>
#include <client/globals.hh>
#include <client/input.hh>
#include <client/screen.hh>
#include <shared/inertial.hh>
#include <shared/vdef.hh>
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

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
    globals::dispatcher.sink<ScreenSizeEvent>().connect<&on_screen_size>();
}

void client_game::init_late()
{
    screen::init_late();

    vdef::assign("grass", 0x0001);

    if(VoxelInfo *info = vdef::find(make_voxel(0x0001, 0))) {
        spdlog::info("{} info:", info->name);
        spdlog::info("  draw: {}", info->draw);
        spdlog::info("  north: {}", info->textures[VOXEL_FACE_NORTH].path.string());
        spdlog::info("  south: {}", info->textures[VOXEL_FACE_SOUTH].path.string());
        spdlog::info("  top: {}", info->textures[VOXEL_FACE_TOP].path.string());
        spdlog::info("  bottom: {}", info->textures[VOXEL_FACE_BOTTOM].path.string());
    }
}

void client_game::deinit()
{

}

void client_game::update()
{

    inertial::update(globals::world, globals::frametime);
}

void client_game::update_late()
{

}

void client_game::render()
{

}
