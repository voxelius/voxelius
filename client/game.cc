// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/camera.hh>
#include <client/canvas_font.hh>
#include <client/canvas.hh>
#include <client/deferred_pass.hh>
#include <client/event/window_resize.hh>
#include <client/final_pass.hh>
#include <client/game.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/options.hh>
#include <client/player_look.hh>
#include <client/player_move.hh>
#include <client/shaders.hh>
#include <client/ui_imgui.hh>
#include <client/ui_main_menu.hh>
#include <client/ui_options.hh>
#include <client/ui_screen.hh>
#include <client/ui_server_list.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
#include <client/voxel_vertex.hh>
#include <GLFW/glfw3.h>
#include <shared/chunks.hh>
#include <shared/inertial.hh>

static void on_window_resize(const WindowResizeEvent &event)
{
    globals::ui_scale = cxmath::max(1U, cxmath::floor<unsigned int>(static_cast<double>(event.height) / 240.0));

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
    options::load();

    shaders::init();
    VoxelVertex::init();

    player_look::init();
    player_move::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

    deferred_pass::init();
    final_pass::init();

    if(!globals::font_8px.load("/textures/font/cga_8x8.png", 8, 8))
    if(!globals::font_8px.load("/textures/font/msx_8x8.png", 8, 8)) {
        spdlog::critical("font_8px: unable to locate a valid font atlas");
        std::terminate();
    }

    if(!globals::font_16px.load("/textures/font/unscii_8x16.png", 8, 16))
    if(!globals::font_16px.load("/textures/font/unifont_8x16.png", 8, 16))
    if(!globals::font_16px.load("/textures/font/vga_8x16.png", 8, 16)) {
        spdlog::critical("font_16px: unable to locate a valid font atlas");
        std::terminate();
    }

    canvas::init();

    ui::imgui::init();

    ui::main_menu::init();
    ui::server_list::init();
    ui::options::init();

    // We start in the main menu
    globals::ui_screen = ui::SCREEN_MAIN_MENU;

    globals::dispatcher.sink<WindowResizeEvent>().connect<&on_window_resize>();
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

    ui::options::deinit();
    ui::server_list::deinit();
    ui::main_menu::deinit();

    canvas::deinit();

    globals::font_16px.unload();
    globals::font_8px.unload();

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

    options::save();
}

void client_game::update()
{
    player_move::update();

    inertial::update(globals::frametime);

    camera::update();

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
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, options::controls::mouse_rawinput);
    }

    ui::imgui::update_late();
}

void client_game::render()
{
    voxel_renderer::render();
    deferred_pass::render();
    final_pass::render();
}

void client_game::render_ui()
{
    if(globals::ui_screen) {
        if(!globals::registry.valid(globals::player)) {
            const double cv = 0.5 + 0.5 * cos(0.25 * globals::curtime * 1.0e-6);
            const double sv = 0.5 + 0.5 * sin(0.25 * globals::curtime * 1.0e-6);
            const vector4_t colz = {cv, sv, 1.0, 1.0};
            const vector4_t colw = {1.0, cv, sv, 1.0};
            canvas::draw_rect_v(0, 0, globals::window_width, globals::window_height, colz, colw);
        }

        const vector4_t spx = {0.0, 0.0, 0.0, 1.0};
        const vector4_t spy = {0.0, 0.0, 0.0, 0.0};
        canvas::draw_rect_h(0, 0, globals::window_width, globals::window_height, spx, spy);
        canvas::draw_rect_h(0, 0, globals::window_width, globals::window_height, spy, spx);

        switch(globals::ui_screen) {
            case ui::SCREEN_MAIN_MENU:
                ui::main_menu::render_ui();
                break;
            case ui::SCREEN_SERVER_LIST:
                ui::server_list::render_ui();
                break;
            case ui::SCREEN_OPTIONS:
                ui::options::render_ui();
                break;
        }
    }
}
