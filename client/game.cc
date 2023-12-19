// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/background.hh>
#include <client/camera.hh>
#include <client/event/glfw_mouse_button.hh>
#include <client/event/glfw_framebuffer_size.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/glxx/framebuffer.hh>
#include <client/lang.hh>
#include <client/player_look.hh>
#include <client/player_move.hh>
#include <client/screenshot.hh>
#include <client/shaders.hh>
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
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <shared/config/boolean.hh>
#include <shared/config/config.hh>
#include <shared/config/number.hh>
#include <shared/config/string.hh>
#include <shared/inertial.hh>
#include <shared/ray_dda.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

config::Boolean client_game::menu_background = config::Boolean{true};
config::Number<unsigned int> client_game::pixel_size = config::Number<unsigned int>{4U};
config::String client_game::username = config::String{"player"};

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

    const double height_base = 240.0;
    const double height_float = event.height;
    const unsigned int scale = cxmath::max(1U, cxmath::floor<unsigned int>(height_float / height_base));

    if(globals::ui_scale != scale) {
        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();

        ImFontConfig font_config = {};
        font_config.FontDataOwnedByAtlas = false;

        io.Fonts->Clear();

        ImFontGlyphRangesBuilder builder = {};
        std::vector<uint8_t> fontbin = {};

        // This should cover a hefty range of glyph ranges.
        // UNDONE: just slap the whole UNICODE Plane-0 here?
        builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
        builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
        builder.AddRanges(io.Fonts->GetGlyphRangesGreek());
        builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());

        ImVector<ImWchar> ranges = {};
        builder.BuildRanges(&ranges);

        if(!vfs::read_bytes("/fonts/SourceCodePro-Regular.ttf", fontbin))
            std::terminate();
        io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 16.0f * scale, &font_config, ranges.Data);

        if(!vfs::read_bytes("/fonts/din1451alt.ttf", fontbin))
            std::terminate();
        globals::font_menu_title = io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 64.0f * scale, &font_config);

        if(!vfs::read_bytes("/fonts/PTMono-Regular.ttf", fontbin))
            std::terminate();
        globals::font_menu_button = io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 16.0f * scale, &font_config, ranges.Data);

        // This should be here!!! Just calling Build()
        // on the font atlas does not invalidate internal
        // device objects defined by the implementation!!!
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        if(globals::ui_scale) {
            // Well, ImGuiStyle::ScaleAllSizes indeed takes
            // the scale values as a RELATIVE scaling, not as
            // absolute. So I have to make a special crutch
            style.ScaleAllSizes(static_cast<float>(scale) / static_cast<float>(globals::ui_scale));
        }

        globals::ui_scale = scale;
    }
}

void client_game::init()
{
    config::add("game.menu_background", client_game::menu_background);
    config::add("game.pixel_size", client_game::pixel_size);
    config::add("game.username", client_game::username);

    lang::init();

    shaders::init();
    VoxelVertex::init();

    player_look::init();
    player_move::init();

    screenshot::init();

    camera::init();

    voxel_anims::init();
    voxel_mesher::init();
    voxel_renderer::init();

    ImGuiStyle &style = ImGui::GetStyle();

    // Rounding on elements looks cool but I am
    // aiming for a more or less blocky and
    // visually simple HiDPI-friendly UI style
    style.TabRounding       = 0.0f;
    style.GrabRounding      = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.WindowRounding    = 0.0f;
    style.ScrollbarRounding = 0.0f;

    style.Colors[ImGuiCol_Text]                     = ImVec4{1.00f, 1.00f, 1.00f, 1.00f};
    style.Colors[ImGuiCol_TextDisabled]             = ImVec4{0.50f, 0.50f, 0.50f, 1.00f};
    style.Colors[ImGuiCol_WindowBg]                 = ImVec4{0.06f, 0.06f, 0.06f, 0.94f};
    style.Colors[ImGuiCol_ChildBg]                  = ImVec4{0.00f, 0.00f, 0.00f, 0.00f};
    style.Colors[ImGuiCol_PopupBg]                  = ImVec4{0.08f, 0.08f, 0.08f, 0.94f};
    style.Colors[ImGuiCol_Border]                   = ImVec4{0.43f, 0.43f, 0.50f, 0.50f};
    style.Colors[ImGuiCol_BorderShadow]             = ImVec4{0.00f, 0.00f, 0.00f, 0.00f};
    style.Colors[ImGuiCol_FrameBg]                  = ImVec4{0.00f, 0.00f, 0.00f, 0.54f};
    style.Colors[ImGuiCol_FrameBgHovered]           = ImVec4{0.36f, 0.36f, 0.36f, 0.40f};
    style.Colors[ImGuiCol_FrameBgActive]            = ImVec4{0.63f, 0.63f, 0.63f, 0.67f};
    style.Colors[ImGuiCol_TitleBg]                  = ImVec4{0.04f, 0.04f, 0.04f, 1.00f};
    style.Colors[ImGuiCol_TitleBgActive]            = ImVec4{0.00f, 0.00f, 0.00f, 1.00f};
    style.Colors[ImGuiCol_TitleBgCollapsed]         = ImVec4{0.00f, 0.00f, 0.00f, 0.51f};
    style.Colors[ImGuiCol_MenuBarBg]                = ImVec4{0.14f, 0.14f, 0.14f, 1.00f};
    style.Colors[ImGuiCol_ScrollbarBg]              = ImVec4{0.02f, 0.02f, 0.02f, 0.53f};
    style.Colors[ImGuiCol_ScrollbarGrab]            = ImVec4{0.00f, 0.00f, 0.00f, 0.75f};
    style.Colors[ImGuiCol_ScrollbarGrabHovered]     = ImVec4{0.12f, 0.12f, 0.12f, 1.00f};
    style.Colors[ImGuiCol_ScrollbarGrabActive]      = ImVec4{0.25f, 0.25f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_CheckMark]                = ImVec4{1.00f, 1.00f, 1.00f, 1.00f};
    style.Colors[ImGuiCol_SliderGrab]               = ImVec4{0.81f, 0.81f, 0.81f, 0.75f};
    style.Colors[ImGuiCol_SliderGrabActive]         = ImVec4{0.00f, 0.00f, 0.00f, 1.00f};
    style.Colors[ImGuiCol_Button]                   = ImVec4{0.00f, 0.00f, 0.00f, 0.75f};
    style.Colors[ImGuiCol_ButtonHovered]            = ImVec4{0.12f, 0.12f, 0.12f, 1.00f};
    style.Colors[ImGuiCol_ButtonActive]             = ImVec4{0.25f, 0.25f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_Header]                   = ImVec4{0.00f, 0.00f, 0.00f, 0.75f};
    style.Colors[ImGuiCol_HeaderHovered]            = ImVec4{0.12f, 0.12f, 0.12f, 1.00f};
    style.Colors[ImGuiCol_HeaderActive]             = ImVec4{0.25f, 0.25f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_Separator]                = ImVec4{0.49f, 0.49f, 0.49f, 0.50f};
    style.Colors[ImGuiCol_SeparatorHovered]         = ImVec4{0.56f, 0.56f, 0.56f, 0.78f};
    style.Colors[ImGuiCol_SeparatorActive]          = ImVec4{0.90f, 0.90f, 0.90f, 1.00f};
    style.Colors[ImGuiCol_ResizeGrip]               = ImVec4{0.34f, 0.34f, 0.34f, 0.20f};
    style.Colors[ImGuiCol_ResizeGripHovered]        = ImVec4{0.57f, 0.57f, 0.57f, 0.67f};
    style.Colors[ImGuiCol_ResizeGripActive]         = ImVec4{1.00f, 1.00f, 1.00f, 0.95f};
    style.Colors[ImGuiCol_Tab]                      = ImVec4{0.00f, 0.00f, 0.00f, 0.75f};
    style.Colors[ImGuiCol_TabHovered]               = ImVec4{0.12f, 0.12f, 0.12f, 1.00f};
    style.Colors[ImGuiCol_TabActive]                = ImVec4{0.25f, 0.25f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_TabUnfocused]             = ImVec4{0.13f, 0.13f, 0.13f, 0.97f};
    style.Colors[ImGuiCol_TabUnfocusedActive]       = ImVec4{0.44f, 0.44f, 0.44f, 1.00f};
    style.Colors[ImGuiCol_PlotLines]                = ImVec4{0.61f, 0.61f, 0.61f, 1.00f};
    style.Colors[ImGuiCol_PlotLinesHovered]         = ImVec4{1.00f, 0.43f, 0.35f, 1.00f};
    style.Colors[ImGuiCol_PlotHistogram]            = ImVec4{0.90f, 0.70f, 0.00f, 1.00f};
    style.Colors[ImGuiCol_PlotHistogramHovered]     = ImVec4{1.00f, 0.60f, 0.00f, 1.00f};
    style.Colors[ImGuiCol_TableHeaderBg]            = ImVec4{0.19f, 0.19f, 0.20f, 1.00f};
    style.Colors[ImGuiCol_TableBorderStrong]        = ImVec4{0.31f, 0.31f, 0.35f, 1.00f};
    style.Colors[ImGuiCol_TableBorderLight]         = ImVec4{0.23f, 0.23f, 0.25f, 1.00f};
    style.Colors[ImGuiCol_TableRowBg]               = ImVec4{0.00f, 0.00f, 0.00f, 0.00f};
    style.Colors[ImGuiCol_TableRowBgAlt]            = ImVec4{1.00f, 1.00f, 1.00f, 0.06f};
    style.Colors[ImGuiCol_TextSelectedBg]           = ImVec4{0.61f, 0.61f, 0.61f, 0.35f};
    style.Colors[ImGuiCol_DragDropTarget]           = ImVec4{1.00f, 1.00f, 0.00f, 0.90f};
    style.Colors[ImGuiCol_NavHighlight]             = ImVec4{0.50f, 0.50f, 0.50f, 1.00f};
    style.Colors[ImGuiCol_NavWindowingHighlight]    = ImVec4{1.00f, 1.00f, 1.00f, 0.70f};
    style.Colors[ImGuiCol_NavWindowingDimBg]        = ImVec4{0.80f, 0.80f, 0.80f, 0.20f};
    style.Colors[ImGuiCol_ModalWindowDimBg]         = ImVec4{0.80f, 0.80f, 0.80f, 0.35f};

    // Making my own Game UI for Source Engine
    // taught me one important thing: dimensions
    // of UI elements must be calculated at semi-runtime
    // so there's simply no point for an INI file.
    ImGui::GetIO().IniFilename = nullptr;

    background::init();

    ui::main_menu::init();
    ui::server_list::init();
    ui::settings::init();

    globals::ui_scale = 0U;
    globals::ui_screen = ui::SCREEN_MAIN_MENU;

    globals::dispatcher.sink<GlfwMouseButtonEvent>().connect<&on_glfw_mouse_button>();
    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
}

void client_game::init_late()
{
    lang::init_late();
}

void client_game::deinit()
{
    voxel_atlas::destroy();

    globals::world_fbo_depth.destroy();
    globals::world_fbo_color.destroy();
    globals::world_fbo.destroy();

    background::deinit();

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
    if(!globals::registry.valid(globals::player)) {
        if(client_game::menu_background.value) {
            background::render();
        }
        else {
            glClearColor(0.25f, 0.25f, 0.25f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }

    if(globals::ui_screen) {
        const float width_f = static_cast<float>(globals::width);
        const float height_f = static_cast<float>(globals::height);
        const ImU32 splash = ImGui::GetColorU32(ImVec4{0.000, 0.000, 0.000, 0.800});
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2{}, ImVec2{width_f, height_f}, splash);

        switch(globals::ui_screen) {
            case ui::SCREEN_MAIN_MENU:
                ui::main_menu::layout();
                break;
            case ui::SCREEN_SERVER_LIST:
                ui::server_list::layout();
                break;
            case ui::SCREEN_SETTINGS:
                ui::settings::layout();
                break;
        }
    }
}
