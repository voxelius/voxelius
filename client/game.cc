// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/camera.hh>
#include <client/canvas_font.hh>
#include <client/canvas.hh>
#include <client/event/mouse_button.hh>
#include <client/event/framebuffer_size.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/glxx/framebuffer.hh>
#include <client/player_look.hh>
#include <client/player_move.hh>
#include <client/screenshot.hh>
#include <client/shaders.hh>
#include <client/ui_imgui.hh>
#include <client/ui_main_menu.hh>
#include <client/ui_screen.hh>
#include <client/ui_server_list.hh>
#include <client/ui_settings.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
#include <client/voxel_vertex.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <shared/config/boolean.hh>
#include <shared/inertial.hh>
#include <shared/ray_dda.hh>
#include <shared/vcollide.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

static void on_mouse_button(const MouseButtonEvent &event)
{
    if(event.action == GLFW_PRESS && event.button == GLFW_MOUSE_BUTTON_LEFT) {
        RayDDA ray = RayDDA{camera::get_position(), camera::get_direction()};
        if(ray.propagate(7.0)) {
            world::set_voxel(ray.vpos, NULL_VOXEL);
        }
    }
}

static void on_framebuffer_size(const FramebufferSizeEvent &event)
{
    globals::world_fbo.create();
    globals::world_fbo_color.create();
    globals::world_fbo_depth.create();
    globals::world_fbo_color.storage(event.width, event.height, PixelFormat::R8G8B8A8_UNORM);
    globals::world_fbo_depth.storage(event.width, event.height, PixelFormat::D24_UNORM);
    globals::world_fbo.attach(GL_COLOR_ATTACHMENT0, globals::world_fbo_color);
    globals::world_fbo.attach(GL_DEPTH_ATTACHMENT, globals::world_fbo_depth);
    globals::world_fbo.set_fragment_targets(GL_COLOR_ATTACHMENT0);

    const double norm = 240.0;
    const double fheight = event.height;
    globals::ui_scale = cxmath::max(1U, cxmath::floor<unsigned int>(fheight / norm));
}

void client_game::init()
{
    shaders::init();
    VoxelVertex::init();

    player_look::init();
    player_move::init();

    screenshot::init();

    camera::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

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
    ui::settings::init();

    // We start in the main menu
    globals::ui_screen = ui::SCREEN_MAIN_MENU;

    globals::dispatcher.sink<MouseButtonEvent>().connect<&on_mouse_button>();
    globals::dispatcher.sink<FramebufferSizeEvent>().connect<&on_framebuffer_size>();
}

void client_game::init_late()
{

}

void client_game::deinit()
{
    voxel_atlas::destroy();

    globals::world_fbo_depth.destroy();
    globals::world_fbo_color.destroy();
    globals::world_fbo.destroy();

    ui::settings::deinit();
    ui::server_list::deinit();
    ui::main_menu::deinit();

    canvas::deinit();

    globals::font_16px.unload();
    globals::font_8px.unload();

    voxel_renderer::deinit();
    voxel_mesher::deinit();
    voxel_anims::deinit();

    world::remove_all_chunks();

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

    vcollide::update(globals::frametime);
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
    else if(globals::registry.valid(globals::player)) {
        glfwSetInputMode(globals::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(globals::window, GLFW_RAW_MOUSE_MOTION, player_look::raw_input.value);
    }

    ui::imgui::update_late();
}

void client_game::render()
{
    glViewport(0, 0, globals::width, globals::height);
    glClearColor(0.529, 0.808, 0.922, 1.000);
    glBindFramebuffer(GL_FRAMEBUFFER, globals::world_fbo.get());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    voxel_renderer::render();

    glViewport(0, 0, globals::width, globals::height);
    glClearColor(0.000, 0.000, 0.000, 1.000);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBlitNamedFramebuffer(globals::world_fbo.get(), 0, 0, 0, globals::width, globals::height, 0, 0, globals::width, globals::height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    canvas::prepare();

    if(globals::ui_screen) {
        if(!globals::registry.valid(globals::player)) {
            const double cv = 0.5 + 0.5 * cos(0.25 * globals::curtime * 1.0e-6);
            const double sv = 0.5 + 0.5 * sin(0.25 * globals::curtime * 1.0e-6);
            const glm::dvec4 colz = {cv, sv, 1.0, 1.0};
            const glm::dvec4 colw = {1.0, cv, sv, 1.0};
            canvas::draw_rect_v(0, 0, globals::width, globals::height, colz, colw);
        }

        const glm::dvec4 spx = {0.0, 0.0, 0.0, 1.0};
        const glm::dvec4 spy = {0.0, 0.0, 0.0, 0.0};
        canvas::draw_rect_h(0, 0, globals::width, globals::height, spx, spy);
        canvas::draw_rect_h(0, 0, globals::width, globals::height, spy, spx);

        switch(globals::ui_screen) {
            case ui::SCREEN_MAIN_MENU:
                ui::main_menu::render();
                break;
            case ui::SCREEN_SERVER_LIST:
                ui::server_list::render();
                break;
            case ui::SCREEN_SETTINGS:
                ui::settings::render();
                break;
        }
    }
}
