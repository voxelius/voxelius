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
#include <client/render/voxel_anims.hh>
#include <client/render/voxel_atlas.hh>
#include <client/render/voxel_mesher.hh>
#include <client/render/voxel_renderer.hh>
#include <client/render/voxel_vertex.hh>
#include <client/ui/imgui.hh>
#include <client/ui/main_menu.hh>
#include <client/ui/screen.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <shared/world/chunks.hh>
#include <shared/world/inertial.hh>

#include <GLFW/glfw3.h>

static void on_framebuffer_size(const FramebufferSizeEvent &event)
{
    const double norm = 480.0;
    const double dheight = event.height;
    globals::ui_scale = cxmath::max(2U, cxmath::ceil<unsigned int>(dheight / norm));

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

    // UNDONE: find a 8x8 bitmap unicode font
    // UNDONE: maybe possibly make our own
    const vfs::path_t font_8x8_path = "/fonts/pc_vga_8x8.bin";
    if(!globals::font_8x8.load_vga_rom(font_8x8_path, 8, 8)) {
        spdlog::critical("{}: load failed", font_8x8_path.string());
        std::terminate();
    }

    const vfs::path_t font_8x16_path = "/fonts/univga_8x16.png";
    if(!globals::font_8x16.load_image(font_8x16_path, 8, 16)) {
        spdlog::critical("{}: load failed", font_8x16_path.string());
        std::terminate();
    }

    // Unifont manages the niche of a 16x16 bitmap font just fine
    const vfs::path_t font_16x16_path = "/fonts/unifont_16x16.png";
    if(!globals::font_16x16.load_image(font_16x16_path, 16, 16)) {
        spdlog::critical("{}: load failed", font_16x16_path.string());
        std::terminate();
    }

    canvas::init();

    imgui::init();

    main_menu::init();

    // We start in the main menu
    globals::ui_screen = ui::SCR_MAIN_MENU;

    globals::dispatcher.sink<FramebufferSizeEvent>().connect<&on_framebuffer_size>();
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

    main_menu::deinit();

    canvas::deinit();

    globals::font_16x16.unload();
    globals::font_8x16.unload();
    globals::font_8x8.unload();

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
    if(globals::ui_screen) {
        glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, false);
    }
    else {
        glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, true);
    }

    imgui::update_late();
}

void client_game::render()
{
    voxel_renderer::render();
    deferred_pass::render();
    final_pass::render();
}

void client_game::draw_gui()
{
    if(globals::ui_screen) {
        if(globals::registry.valid(globals::player)) {
            const vector4_t spx = {0.0, 0.0, 0.0, 0.99};
            const vector4_t spy = {0.0, 0.0, 0.0, 0.75};
            canvas::rect_h(0, 0, globals::window_width, globals::window_height, spx, spy);
        }

        switch(globals::ui_screen) {
            case ui::SCR_MAIN_MENU:
                main_menu::draw_ui();
                break;
            case ui::SCR_SERVER_BROWSER:
                break;
            case ui::SCR_SETTINGS:
                break;
        }
    }
}
