// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/atlas.hh>
#include <client/deferred.hh>
#include <client/game.hh>
#include <client/gameui_menu.hh>
#include <client/gameui_servers.hh>
#include <client/gameui_settings.hh>
#include <client/gbuffer.hh>
#include <client/glfw_events.hh>
#include <client/globals.hh>
#include <client/pm_look.hh>
#include <client/pm_move.hh>
#include <client/postprocess.hh>
#include <client/shaders.hh>
#include <client/ui_draw.hh>
#include <client/ui_font.hh>
#include <client/ui_immediate.hh>
#include <client/view.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
#include <client/voxel_vertex.hh>
#include <shared/inertial.hh>
#include <spdlog/spdlog.h>

static void on_mouse_button(const MouseButtonEvent &event)
{
    if(!globals::gameui_visible && event.action == GLFW_PRESS) {
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

static void on_framebuffer_size(const FramebufferSizeEvent &event)
{
    const double dheight = event.height;
    globals::gameui_scale = cxmath::max(1U, cxmath::ceil<unsigned int>(dheight / 240.0));

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

    pm_look::init();
    pm_move::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

    deferred::init();
    postprocess::init();

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

    ui::draw::init();
    ui::im::init();

    gameui_menu::init();
    gameui_servers::init();
    gameui_settings::init();

    // Start in main menu
    globals::gameui_screen = GameUiScreen::MainMenu;
    globals::gameui_visible = true;

    globals::dispatcher.sink<MouseButtonEvent>().connect<&on_mouse_button>();
    globals::dispatcher.sink<FramebufferSizeEvent>().connect<&on_framebuffer_size>();
}

void client_game::init_late()
{

}

void client_game::deinit()
{
    atlas::destroy();

    globals::deferred_fbo.destroy();
    globals::deferred_color.destroy();

    globals::gbuffer_blend.destroy();
    globals::gbuffer_cutout.destroy();
    globals::gbuffer_solid.destroy();

    gameui_settings::deinit();
    gameui_servers::deinit();
    gameui_menu::deinit();

    ui::draw::deinit();

    globals::pc_vga_8x8.unload();
    globals::pc_vga_8x16.unload();
    globals::unifont_16x16.unload();

    postprocess::deinit();
    deferred::deinit();

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
    int cursor_mode = GLFW_CURSOR_DISABLED;
    if(globals::gameui_screen != GameUiScreen::Undefined && globals::gameui_visible)
        cursor_mode = GLFW_CURSOR_NORMAL;
    glfwSetInputMode(globals::window, GLFW_CURSOR, cursor_mode);

    ui::im::update_late();
}

void client_game::render()
{
    voxel_renderer::render();
    deferred::render();
    postprocess::render();
}

void client_game::draw_ui()
{
    if(globals::gameui_visible) {
        if(globals::world.registry.valid(globals::player)) {
            const vector4_t splash = {0.0, 0.0, 0.0, 0.75};
            ui::draw::rect(0, 0, globals::window_width, globals::window_height, splash);
        }

        switch(globals::gameui_screen) {
            case GameUiScreen::MainMenu:
                gameui_menu::draw_ui();
                break;
            case GameUiScreen::ServerBrowser:
                gameui_servers::draw_ui();
                break;
            case GameUiScreen::Settings:
                gameui_settings::draw_ui();
                break;
        }
    }
}
