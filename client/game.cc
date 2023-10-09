// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/framebuffer_size.hh>
#include <client/player/player_look.hh>
#include <client/player/player_move.hh>
#include <client/render/canvas.hh>
#include <client/render/deferred_pass.hh>
#include <client/render/final_pass.hh>
#include <client/render/font.hh>
#include <client/render/gbuffer.hh>
#include <client/render/shaders.hh>
#include <client/render/view.hh>
#include <client/render/voxel_anims.hh>
#include <client/render/voxel_atlas.hh>
#include <client/render/voxel_mesher.hh>
#include <client/render/voxel_renderer.hh>
#include <client/render/voxel_vertex.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <shared/world/chunks.hh>
#include <shared/world/inertial.hh>

static void on_framebuffer_size(const FramebufferSizeEvent &event)
{
    globals::gbuffer_solid.create(event.width, event.height);
    globals::gbuffer_cutout.create(event.width, event.height);
    globals::gbuffer_blend.create(event.width, event.height);

    globals::deferred_color.create();
    globals::deferred_color.storage(event.width, event.height, PixelFormat::R8G8B8A8_UNORM);
    globals::deferred_fbo.create();
    globals::deferred_fbo.attach(GL_COLOR_ATTACHMENT0, globals::deferred_color);
    globals::deferred_fbo.set_fragment_targets(GL_COLOR_ATTACHMENT0);
}

void client_game::init()
{
    shaders::init();
    VoxelVertex::init();

    player_look::init();
    player_move::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

    deferred_pass::init();
    final_pass::init();

    const vfs::path_t unifont_16x16_path = "/fonts/unifont_16x16.png";
    if(!globals::unifont_16x16.load_image(unifont_16x16_path, 16, 16)) {
        spdlog::critical("{}: load failed", unifont_16x16_path.string());
        std::terminate();
    }

    const vfs::path_t pc_vga_8x16_path = "/fonts/pc_vga_8x16.bin";
    if(!globals::pc_vga_8x16.load_vga_rom(pc_vga_8x16_path, 8, 16)) {
        spdlog::critical("{}: load failed", pc_vga_8x16_path.string());
        std::terminate();
    }

    const vfs::path_t pc_vga_8x8_path = "/fonts/pc_vga_8x8.bin";
    if(!globals::pc_vga_8x8.load_vga_rom(pc_vga_8x8_path, 8, 8)) {
        spdlog::critical("{}: load failed", pc_vga_8x8_path.string());
        std::terminate();
    }

    canvas::init();
}

void client_game::init_late()
{

}

void client_game::deinit()
{
    voxel_atlas::destroy();

    globals::deferred_fbo.destroy();
    globals::deferred_color.destroy();

    globals::gbuffer_blend.destroy();
    globals::gbuffer_cutout.destroy();
    globals::gbuffer_solid.destroy();

    canvas::deinit();

    globals::pc_vga_8x8.unload();
    globals::pc_vga_8x16.unload();
    globals::unifont_16x16.unload();

    final_pass::deinit();
    deferred_pass::deinit();

    voxel_renderer::deinit();
    voxel_mesher::deinit();
    voxel_anims::deinit();

    chunks::remove_all();

    // Certain components' fields
    // have destructors to call OpenGL
    // functions, which are clobbered
    // by the time destructor on globals::registry
    // is called, turning shutdown into a segfault
    globals::registry.clear();
}

void client_game::update()
{
    player_move::update();

    inertial::update(globals::frametime);

    view::update();

    voxel_anims::update();
    voxel_mesher::update();
}

void client_game::update_late()
{

}

void client_game::render()
{
    voxel_renderer::render();
    deferred_pass::render();
    final_pass::render();
}

void client_game::draw_gui()
{
    canvas::rect_h(0, 0, globals::window_width, globals::window_height, {1.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0});
}
