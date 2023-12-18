// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/camera.hh>
#include <client/event/glfw_mouse_button.hh>
#include <client/event/glfw_framebuffer_size.hh>
#include <client/event/imgui_fonts_reload.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/glxx/framebuffer.hh>
#include <client/player_look.hh>
#include <client/player_move.hh>
#include <client/screenshot.hh>
#include <client/shaders.hh>
#include <client/ui_main_menu.hh>
#include <client/ui_screen.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <client/voxel_mesher.hh>
#include <client/voxel_renderer.hh>
#include <client/voxel_vertex.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <shared/config/boolean.hh>
#include <shared/config/config.hh>
#include <shared/config/number.hh>
#include <shared/inertial.hh>
#include <shared/ray_dda.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

config::Number<unsigned int> client_game::pixel_size = 4U;

static void on_glfw_mouse_button(const GlfwMouseButtonEvent &event)
{

}

static void on_glfw_framebuffer_size(const GlfwFramebufferSizeEvent &event)
{
    globals::world_fbo.create();
    globals::world_fbo_color.create();
    globals::world_fbo_depth.create();
    globals::world_fbo_color.storage(event.width, event.height, PixelFormat::R8G8B8A8_UNORM);
    globals::world_fbo_depth.storage(event.width, event.height, PixelFormat::D24_UNORM);
    globals::world_fbo.attach(GL_COLOR_ATTACHMENT0, globals::world_fbo_color);
    globals::world_fbo.attach(GL_DEPTH_ATTACHMENT, globals::world_fbo_depth);
    globals::world_fbo.set_fragment_targets(GL_COLOR_ATTACHMENT0);
}

static void on_imgui_fonts_reload(const ImGuiFontsReloadEvent &event)
{
    std::vector<uint8_t> font = {};

    if(!vfs::read_bytes("/fonts/SourceCodePro-Regular.ttf", font))
        std::terminate();
    globals::font_default = event.fonts->AddFontFromMemoryTTF(font.data(), font.size(), 16.0f * event.scale, &event.config);

    if(!vfs::read_bytes("/fonts/din1451alt.ttf", font))
        std::terminate();
    globals::font_menu_title = event.fonts->AddFontFromMemoryTTF(font.data(), font.size(), 64.0f * event.scale, &event.config);

    if(!vfs::read_bytes("/fonts/PTMono-Regular.ttf", font))
        std::terminate();
    globals::font_menu_button = event.fonts->AddFontFromMemoryTTF(font.data(), font.size(), 16.0f * event.scale, &event.config);
}

void client_game::init()
{
    config::add("game.pixel_size", client_game::pixel_size);

    shaders::init();
    VoxelVertex::init();

    player_look::init();
    player_move::init();

    screenshot::init();

    camera::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

    ui::main_menu::init();

    // We want to start in the main menu
    globals::ui_screen = ui::SCREEN_MAIN_MENU;

    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
    globals::dispatcher.sink<ImGuiFontsReloadEvent>().connect<&on_imgui_fonts_reload>();
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
}

void client_game::render()
{
    const int scaled_width = globals::width / client_game::pixel_size.value;
    const int scaled_height = globals::height / client_game::pixel_size.value;

    glViewport(0, 0, scaled_width, scaled_height);
    glClearColor(0.529, 0.808, 0.922, 1.000);
    glBindFramebuffer(GL_FRAMEBUFFER, globals::world_fbo.get());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    voxel_renderer::render();

    glViewport(0, 0, globals::width, globals::height);
    glClearColor(0.000, 0.000, 0.000, 1.000);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlitNamedFramebuffer(globals::world_fbo.get(), 0, 0, 0, scaled_width, scaled_height, 0, 0, globals::width, globals::height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void client_game::layout()
{
    ImGui::PushFont(globals::font_default);

    if(globals::ui_screen) {
        const float width_f = static_cast<float>(globals::width);
        const float height_f = static_cast<float>(globals::height);
        const ImU32 splash = ImGui::GetColorU32(ImVec4{0.000, 0.000, 0.000, 0.800});
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2{}, ImVec2{width_f, height_f}, splash);

        switch(globals::ui_screen) {
            case ui::SCREEN_MAIN_MENU:
                ui::main_menu::layout();
                break;
        }
    }

    ImGui::PopFont();
}
