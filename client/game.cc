// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/game.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/input.hh>
#include <client/player_look.hh>
#include <client/player_move.hh>
#include <client/screen.hh>
#include <client/view.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
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
    globals::gbuffer_opaque.create(event.width, event.height);
    globals::gbuffer_alpha.create(event.width, event.height);
    globals::gbuffer_fluid.create(event.width, event.height);

    spdlog::trace("{}x{} [{}]", event.width, event.height, event.aspect);
}

void client_game::init()
{
    input::init();
    screen::init();

    player_look::init();
    player_move::init();

    voxel_mesher::init();
    voxel_renderer::init();

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
    globals::dispatcher.sink<ScreenSizeEvent>().connect<&on_screen_size>();
}

void client_game::init_late()
{
    screen::init_late();

    spdlog::info("spawning local player");
    globals::player = globals::world.registry.create();
    globals::world.registry.emplace<PlayerComponent>(globals::player);
    globals::world.registry.emplace<HeadComponent>(globals::player);
    globals::world.registry.emplace<TransformComponent>(globals::player);
    globals::world.registry.emplace<VelocityComponent>(globals::player);

    vdef::assign("grass", 0x000001);

    if(VoxelInfo *info = vdef::find(0x00000001)) {
        spdlog::info("{} info:", info->name);
        spdlog::info("  draw: {}", info->draw);
        spdlog::info("  north: {}", info->textures[VOXEL_FACE_NORTH].path.string());
        spdlog::info("  south: {}", info->textures[VOXEL_FACE_SOUTH].path.string());
        spdlog::info("  top: {}", info->textures[VOXEL_FACE_TOP].path.string());
        spdlog::info("  bottom: {}", info->textures[VOXEL_FACE_BOTTOM].path.string());
    }

    atlas::create(16, 16, vdef::textures.size());

    for(const auto it : vdef::textures) {
        if(!atlas::load(it)) {
            std::terminate();
        }
    }

    for(auto it : vdef::voxels) {
        if(it != nullptr) {
            for(auto &jt : it->vec) {
                for(auto &kt : jt.textures) {
                    const auto *atex = atlas::find(kt.path);
                    if(atex == nullptr)
                        std::terminate();
                    kt.cache = atex->tex_id;
                }
            }
        }
    }

    for(int i = 0; i < 16; ++i) {
        for(int j = 0; j < 16; ++j) {
            for(int k = 0; k < 1; ++k) {
                globals::world.set_voxel(0x00000001, voxel_pos_t{i, k, j});
            }
        }
    }

    for(int i = 0; i < 4; ++i) {
       globals::world.set_voxel(0x00000001, voxel_pos_t{i, 1, i});
    }
}

void client_game::deinit()
{
    globals::world.purge();

    globals::gbuffer_opaque.destroy();
    globals::gbuffer_alpha.destroy();
    globals::gbuffer_fluid.destroy();

    atlas::destroy();

    voxel_renderer::deinit();

    voxel_mesher::deinit();
}

void client_game::update()
{
    player_move::update();

    view::update();

    voxel_mesher::update();

    inertial::update(globals::world, globals::frametime);
}

void client_game::update_late()
{
    glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    const vec3f_t angles = view::get_angles();
    const vec3f_t rang = glm::degrees(angles);
    spdlog::info("ANG: {} {} {}", rang.x, rang.y, rang.z);
}

void client_game::render()
{
    voxel_renderer::render();

    int width, height;
    screen::get_size(width, height);
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBlitNamedFramebuffer(globals::gbuffer_opaque.get_albedo_attachment().get(), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
