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
#include <client/pm_look.hh>
#include <client/pm_move.hh>
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

constexpr static const uint32_t STONE   = 1;
constexpr static const uint32_t SLATE   = 2;
constexpr static const uint32_t DIRT    = 3;
constexpr static const uint32_t GRASS   = 4;
constexpr static const uint32_t VTEST   = 5;

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

static void on_mouse_button(const MouseButtonEvent &event)
{
    if(event.action == GLFW_PRESS) {
        RayDDA ray = {};
        ViewValues vv = {};
        view::get_values(vv);

        if(globals::world.raycast(ray, vv.position, vv.direction, 10.0)) {
            switch(event.button) {
                case GLFW_MOUSE_BUTTON_LEFT:
                    globals::world.set_voxel(ray.vpos, NULL_VOXEL);
                    break;
            }
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

    pm_look::init();
    pm_move::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

    deferred_pass::init();
    postprocess::init();

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
    globals::dispatcher.sink<MouseButtonEvent>().connect<&on_mouse_button>();
    globals::dispatcher.sink<ScreenSizeEvent>().connect<&on_screen_size>();
}

// Surface level for world generation
constexpr static const int64_t SURFACE = 0;

#include <glm/gtc/noise.hpp>

static voxel_t voxel_at(const voxel_pos_t &vpos)
{
    int64_t surf = SURFACE + 8.0 * glm::simplex(vector2_t{vpos.x, vpos.z} / 48.0);
    if(vpos.y <= surf - 8)
        return make_voxel(STONE, NULL_VOXEL_STATE);
    if(vpos.y <= surf - 1)
        return make_voxel(DIRT, NULL_VOXEL_STATE);
    if(vpos.y <= surf)
        return make_voxel(GRASS, NULL_VOXEL_STATE);
    return NULL_VOXEL;
}

static void generate(const chunk_pos_t &cpos)
{
    spdlog::trace("generating {} {} {}", cpos.x, cpos.y, cpos.z);

    bool empty = true;
    voxel_array_t voxels = {};

    for(size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = coord::to_local(i);
        const auto vpos = coord::to_voxel(cpos, lpos);
        const auto voxel = voxel_at(vpos);
        if(voxel != NULL_VOXEL)
            empty = false;
        voxels[i] = voxel;
    }

    if(!empty) {
        Chunk *chunk = globals::world.create_chunk(cpos);
        chunk->voxels = voxels;
    }
}

void client_game::init_late()
{
    screen::init_late();

    vdef::purge();
    vdef::assign("stone", STONE);
    vdef::assign("slate", SLATE);
    vdef::assign("dirt", DIRT);
    vdef::assign("grass", GRASS);
    vdef::assign("vtest", VTEST);

    atlas::create(16, 16, vdef::textures.size());

    for(const vfs::path_t &path : vdef::textures) {
        if(!atlas::load(path)) {
            spdlog::critical("atlas: {}: load failed", path.string());
            std::terminate();
        }
    }

    voxel_anims::construct();

    for(int x = -8; x < 7; ++x) {
        for(int z = -8; z < 7; ++z) {
            for(int y = -1; y < 2; ++y) {
                generate(chunk_pos_t{x, y, z});
            }
        }
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
    pm_move::update();

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
