// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/event/glfw_framebuffer_size.hh>
#include <client/atlas.hh>
#include <client/background.hh>
#include <client/camera.hh>
#include <client/chunk_mesher.hh>
#include <client/chunk_renderer.hh>
#include <client/chunk_vis.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/key_name.hh>
#include <client/keyboard.hh>
#include <client/lang.hh>
#include <client/main_menu.hh>
#include <client/mouse.hh>
#include <client/progress.hh>
#include <client/screenshot.hh>
#include <client/server_list.hh>
#include <client/settings.hh>
#include <client/ui_screen.hh>
#include <client/voxel_anims.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include <shared/util/physfs.hh>
#include <shared/config.hh>
#include <shared/floatfix.hh>
#include <shared/inertial.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

static bool menu_background = true;
static unsigned int pixel_size = 4U;
std::string client_game::username = "player";

static void on_glfw_framebuffer_size(const GlfwFramebufferSizeEvent &event)
{
    if(globals::world_fbo) {
        glDeleteRenderbuffers(1, &globals::world_fbo_depth);
        glDeleteTextures(1, &globals::world_fbo_color);
        glDeleteFramebuffers(1, &globals::world_fbo);
    }

    glGenFramebuffers(1, &globals::world_fbo);
    glGenTextures(1, &globals::world_fbo_color);
    glGenRenderbuffers(1, &globals::world_fbo_depth);

    glBindTexture(GL_TEXTURE_2D, globals::world_fbo_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, event.width, event.height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glBindRenderbuffer(GL_RENDERBUFFER, globals::world_fbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, event.width, event.height);

    glBindFramebuffer(GL_FRAMEBUFFER, globals::world_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globals::world_fbo_color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, globals::world_fbo_depth);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::critical("opengl: world framebuffer is incomplete");
        glDeleteRenderbuffers(1, &globals::world_fbo_depth);
        glDeleteTextures(1, &globals::world_fbo_color);
        glDeleteFramebuffers(1, &globals::world_fbo);
        std::terminate();
    }

    const float height_base = 240.0f;
    const float height_float = event.height;
    const unsigned int scale = util::max(1U, util::floor<unsigned int>(height_float / height_base));

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

        if(!util::read_bytes("fonts/RobotoSlab-Medium.ttf", fontbin))
            std::terminate();
        io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 13.0f * scale, &font_config, ranges.Data);

        if(!util::read_bytes("fonts/din1451alt.ttf", fontbin))
            std::terminate();
        globals::font_menu_title = io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 64.0f * scale, &font_config);

        if(!util::read_bytes("fonts/PTMono-Regular.ttf", fontbin))
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

void client_game::init(void)
{
    Config::add(globals::client_config, "game.menu_background", menu_background);
    Config::add(globals::client_config, "game.pixel_size", pixel_size);
    Config::add(globals::client_config, "game.username", client_game::username);

    settings::add_checkbox(0, settings::VIDEO_GUI, "game.menu_background", menu_background, true);
    settings::add_slider(1, settings::VIDEO, "game.pixel_size", pixel_size, 1U, 4U, true);
    settings::add_input(1, settings::GENERAL, "game.username", client_game::username, false, false);

    lang::init();

    key_name::init();

    keyboard::init();
    mouse::init();

    screenshot::init();

    camera::init();

    voxel_anims::init();

    chunk_mesher::init();
    chunk_renderer::init();

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

    style.Colors[ImGuiCol_Text]                     = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]             = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]                 = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    style.Colors[ImGuiCol_ChildBg]                  = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]                  = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    style.Colors[ImGuiCol_Border]                   = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    style.Colors[ImGuiCol_BorderShadow]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]                  = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered]           = ImVec4(0.36f, 0.36f, 0.36f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive]            = ImVec4(0.63f, 0.63f, 0.63f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]                  = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]            = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]         = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]              = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]            = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]     = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]      = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]               = ImVec4(0.81f, 0.81f, 0.81f, 0.75f);
    style.Colors[ImGuiCol_SliderGrabActive]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_Button]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
    style.Colors[ImGuiCol_ButtonHovered]            = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_Header]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
    style.Colors[ImGuiCol_HeaderHovered]            = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_Separator]                = ImVec4(0.49f, 0.49f, 0.49f, 0.50f);
    style.Colors[ImGuiCol_SeparatorHovered]         = ImVec4(0.56f, 0.56f, 0.56f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]          = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]               = ImVec4(0.34f, 0.34f, 0.34f, 0.20f);
    style.Colors[ImGuiCol_ResizeGripHovered]        = ImVec4(0.57f, 0.57f, 0.57f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]         = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    style.Colors[ImGuiCol_Tab]                      = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
    style.Colors[ImGuiCol_TabHovered]               = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TabActive]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused]             = ImVec4(0.13f, 0.13f, 0.13f, 0.97f);
    style.Colors[ImGuiCol_TabUnfocusedActive]       = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]                = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]         = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]            = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]     = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg]            = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TableBorderStrong]        = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight]         = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_TableRowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_TableRowBgAlt]            = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg]           = ImVec4(0.61f, 0.61f, 0.61f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget]           = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight]             = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight]    = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]        = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg]         = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    // Making my own Game UI for Source Engine
    // taught me one important thing: dimensions
    // of UI elements must be calculated at semi-runtime
    // so there's simply no point for an INI file.
    ImGui::GetIO().IniFilename = nullptr;

    background::init();

    main_menu::init();
    progress::init();
    server_list::init();
    settings::init();

    globals::ui_keybind_ptr = nullptr;
    globals::ui_scale = 0U;
    globals::ui_screen = UI_MAIN_MENU;

    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
}

void client_game::init_late(void)
{
    lang::init_late();

    settings::init_late();
}

void client_game::deinit(void)
{
    atlas::destroy();

    glDeleteRenderbuffers(1, &globals::world_fbo_depth);
    glDeleteTextures(1, &globals::world_fbo_color);
    glDeleteFramebuffers(1, &globals::world_fbo);

    background::deinit();

    chunk_renderer::deinit();
    chunk_mesher::deinit();

    world::purge_chunks();

    // This makes sure there are no
    // objects present that might have
    // an OpenGL object attached to them
    // because it is the last time we're able
    // to safely deallocate anything OpenGL
    globals::registry.clear();
}

void client_game::update(void)
{
    keyboard::update();

    inertial::update(globals::frametime);

    floatfix::update();

    camera::update();

    voxel_anims::update();

    chunk_mesher::update();

    chunk_vis::update();
}

void client_game::update_late(void)
{
    mouse::update_late();
}

void client_game::render(void)
{
    const int scaled_width = globals::width / util::max(1U, pixel_size);
    const int scaled_height = globals::height / util::max(1U, pixel_size);

    glViewport(0, 0, scaled_width, scaled_height);
    glClearColor(0.529f, 0.808f, 0.922f, 1.000f);
    glBindFramebuffer(GL_FRAMEBUFFER, globals::world_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    chunk_renderer::render();

    glViewport(0, 0, globals::width, globals::height);
    glClearColor(0.000f, 0.000f, 0.000f, 1.000f);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, globals::world_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, scaled_width, scaled_height, 0, 0, globals::width, globals::height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void client_game::layout(void)
{
    if(!globals::registry.valid(globals::player)) {
        if(menu_background) {
            background::render();
        }
        else {
            glClearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }

    if(globals::ui_screen) {
        if(globals::registry.valid(globals::player)) {
            const float width_f = static_cast<float>(globals::width);
            const float height_f = static_cast<float>(globals::height);
            const ImU32 splash = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.900f));
            ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(), ImVec2(width_f, height_f), splash);
        }

        switch(globals::ui_screen) {
            case UI_MAIN_MENU:
                main_menu::layout();
                break;
            case UI_SERVER_LIST:
                server_list::layout();
                break;
            case UI_SETTINGS:
                settings::layout();
                break;
            case UI_PROGRESS:
                progress::layout();
                break;
        }
    }
}
