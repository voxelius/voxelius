// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/deferred_pass.hh>
#include <client/game.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/input.hh>
#include <client/player_look.hh>
#include <client/player_move.hh>
#include <client/postprocess.hh>
#include <client/screen.hh>
#include <client/shaders.hh>
#include <client/view.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
#include <client/voxel_vertex.hh>
#include <shared/head.hh>
#include <shared/inertial.hh>
#include <shared/player.hh>
#include <shared/transform.hh>
#include <shared/velocity.hh>
#include <spdlog/spdlog.h>

static void on_key(const KeyEvent &event)
{
    if(event.action == GLFW_PRESS) {
        if(event.key == GLFW_KEY_ESCAPE) {
            spdlog::info("GLFW_KEY_ESCAPE has been pressed, exiting");
            glfwSetWindowShouldClose(globals::window, true);
            return;
        }
    }
}

static void on_screen_size(const ScreenSizeEvent &event)
{
    globals::gbuffer_solid.create(event.width, event.height);
    globals::gbuffer_cutout.create(event.width, event.height);
    globals::gbuffer_blend.create(event.width, event.height);

    globals::deferred_color.create();
    globals::deferred_color.storage(event.width, event.height, glxx::PixelFormat::R8G8B8A8_UNORM);
    globals::deferred_fbo.create();
    globals::deferred_fbo.attach(GL_COLOR_ATTACHMENT0, globals::deferred_color);
    globals::deferred_fbo.set_fragment_targets(GL_COLOR_ATTACHMENT0);
}

void client_game::init()
{
    input::init();
    screen::init();

    shaders::init();
    VoxelVertex::init();

    player_look::init();
    player_move::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

    deferred_pass::init();
    postprocess::init();

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
    globals::dispatcher.sink<ScreenSizeEvent>().connect<&on_screen_size>();
}

void client_game::init_late()
{
    screen::init_late();

    vdef::purge();
    vdef::assign("stone",   0x000001);
    vdef::assign("slate",   0x000002);
    vdef::assign("dirt",    0x000003);
    vdef::assign("grass",   0x000004);
    vdef::assign("test",    0x000005);

    atlas::create(16, 16, vdef::textures.size());

    for(const vfs::path_t &path : vdef::textures) {
        if(!atlas::load(path)) {
            spdlog::critical("atlas: {}: load failed", path.string());
            std::terminate();
        }
    }

    voxel_anims::construct();

    globals::world.create_chunk(chunk_pos_t{0, 0, 0})->voxels.fill(0x000001);
    globals::world.create_chunk(chunk_pos_t{0, 0, 1})->voxels.fill(0x000002);
    globals::world.create_chunk(chunk_pos_t{0, 0, 2})->voxels.fill(0x000003);
    globals::world.create_chunk(chunk_pos_t{0, 0, 3})->voxels.fill(0x000004);
    globals::world.create_chunk(chunk_pos_t{2, 0, 0})->voxels.fill(0x000005);

    for(int x = 0; x < 16; ++x) {
        for(int z = 0; z < 16; ++z) {
            for(int y = 0; y < 2; ++y) {
                globals::world.set_voxel(0x000001, voxel_pos_t{x, 16 + y, z});
            }
        }
    }

    for(int xz = 0; xz < 8; ++xz) {
        globals::world.set_voxel(0x000002, voxel_pos_t{xz / 2, 18, xz});
    }

    spdlog::info("spawning local player");
    globals::player = globals::world.registry.create();
    globals::world.registry.emplace<PlayerComponent>(globals::player);
    globals::world.registry.emplace<HeadComponent>(globals::player);
    globals::world.registry.emplace<TransformComponent>(globals::player);
    globals::world.registry.emplace<VelocityComponent>(globals::player);
}

void client_game::deinit()
{
    atlas::destroy();

    globals::deferred_fbo.destroy();
    globals::deferred_color.destroy();

    globals::gbuffer_blend.destroy();
    globals::gbuffer_cutout.destroy();
    globals::gbuffer_solid.destroy();

    postprocess::deinit();
    deferred_pass::deinit();

    voxel_renderer::deinit();
    voxel_mesher::deinit();
    voxel_anims::deinit();

    // Certain components have their destructors
    // calling OpenGL API functions, which are
    // clobbered past glfwDestroyWindow, so it's
    // a good idea to purge those right now
    globals::world.purge();
}

void client_game::update()
{
    player_move::update();

    inertial::update(globals::world, globals::frametime);

    view::update();

    voxel_anims::update();
    voxel_mesher::update();
}

void client_game::update_late()
{
    // FIXME: there should be a way to release the cursor
    glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void client_game::render()
{
    voxel_renderer::render();
    deferred_pass::render();
    postprocess::render();
}
