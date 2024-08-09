// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/debug_screen.hh>
#include <client/debug_session.hh>
#include <client/debug_toggles.hh>
#include <client/entity/player_look.hh>
#include <client/entity/player_move.hh>
#include <client/event/glfw_framebuffer_size.hh>
#include <client/background.hh>
#include <client/language.hh>
#include <client/main_menu.hh>
#include <client/progress.hh>
#include <client/screen.hh>
#include <client/server_list.hh>
#include <client/settings.hh>
#include <client/chunk_mesher.hh>
#include <client/chunk_renderer.hh>
#include <client/chunk_visibility.hh>
#include <client/player_target.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/keyboard.hh>
#include <client/keynames.hh>
#include <client/mouse.hh>
#include <client/screenshot.hh>
#include <client/outline_renderer.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/util/physfs.hh>
#include <shared/ray_dda.hh>
#include <shared/world.hh>
#include <shared/config.hh>
#include <spdlog/spdlog.h>

bool client_game::vertical_sync = true;
bool client_game::menu_background = true;
std::string client_game::username = "player";
unsigned int client_game::pixel_size = 4U;
unsigned int client_game::fog_mode = 1U;

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

    constexpr float width_base = 360.0f;
    constexpr float height_base = 240.0f;
    const float width_float = event.width;
    const float height_float = event.height;
    const unsigned int wscale = cxpr::max(1U, cxpr::floor<unsigned int>(width_float / width_base));
    const unsigned int hscale = cxpr::max(1U, cxpr::floor<unsigned int>(height_float / height_base));
    const unsigned int scale = cxpr::min(wscale, hscale);

    if(globals::gui_scale != scale) {
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

        if(!util::read_bytes("fonts/unscii-16.ttf", fontbin))
            std::terminate();
        io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 16.0f * scale, &font_config, ranges.Data);

        if(!util::read_bytes("fonts/unscii-8.ttf", fontbin))
            std::terminate();
        globals::font_debug = io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 4.0f * scale, &font_config);

        // UNDONE: design a logo and draw it as a TEXTURE/SPRITE
        if(!util::read_bytes("fonts/din1451alt.ttf", fontbin))
            std::terminate();
        globals::font_menu_title = io.Fonts->AddFontFromMemoryTTF(fontbin.data(), fontbin.size(), 64.0f * scale, &font_config);

        // This should be here!!! Just calling Build()
        // on the font atlas does not invalidate internal
        // device objects defined by the implementation!!!
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        if(globals::gui_scale) {
            // Well, ImGuiStyle::ScaleAllSizes indeed takes
            // the scale values as a RELATIVE scaling, not as
            // absolute. So I have to make a special crutch
            style.ScaleAllSizes(static_cast<float>(scale) / static_cast<float>(globals::gui_scale));
        }

        globals::gui_scale = scale;
    }
}

void client_game::init(void)
{
    Config::add(globals::client_config, "game.vertical_sync", client_game::vertical_sync);
    Config::add(globals::client_config, "game.menu_background", client_game::menu_background);
    Config::add(globals::client_config, "game.username", client_game::username);
    Config::add(globals::client_config, "game.pixel_size", client_game::pixel_size);
    Config::add(globals::client_config, "game.fog_mode", client_game::fog_mode);

    settings::add_checkbox(4, settings::VIDEO, "game.vertical_sync", client_game::vertical_sync, false);
    settings::add_checkbox(0, settings::VIDEO_GUI, "game.menu_background", client_game::menu_background, true);
    settings::add_input(1, settings::GENERAL, "game.username", client_game::username, false, false);
    settings::add_slider(1, settings::VIDEO, "game.pixel_size", client_game::pixel_size, 1U, 4U, true);
    settings::add_stepper(3, settings::VIDEO, "game.fog_mode", client_game::fog_mode, 3U, false);

    language::init();

    player_move::init();
    player_target::init();

    keynames::init();
    keyboard::init();
    mouse::init();

    screenshot::init();

    view::init();

    voxel_anims::init();

    chunk_mesher::init();
    chunk_renderer::init();

    outline_renderer::init();
    
    ImGuiStyle &style = ImGui::GetStyle();

    // Black buttons on a dark background
    // may be harder to read than the text on them
    style.FrameBorderSize = 1.0;

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

    debug_toggles::init();
    debug_screen::init();
    debug_session::init();

    background::init();

    main_menu::init();
    progress::init();
    server_list::init();
    settings::init();

    debug_session::init();

    globals::sky_color[0] = 0.529f;
    globals::sky_color[1] = 0.808f;
    globals::sky_color[2] = 0.922f;
    globals::sky_color[3] = 1.000f;

    globals::gui_keybind_ptr = nullptr;
    globals::gui_scale = 0U;
    globals::gui_screen = GUI_MAIN_MENU;

    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
}

void client_game::init_late(void)
{
    language::init_late();

    settings::init_late();
}

void client_game::deinit(void)
{
    voxel_atlas::destroy();

    glDeleteRenderbuffers(1, &globals::world_fbo_depth);
    glDeleteTextures(1, &globals::world_fbo_color);
    glDeleteFramebuffers(1, &globals::world_fbo);

    background::deinit();

    outline_renderer::deinit();

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
    debug_session::update();

    player_move::update();
    player_target::update();

    VelocityComponent::update(globals::frametime);
    TransformComponent::update();

    view::update();

    voxel_anims::update();

    chunk_mesher::update();

    chunk_visibility::update();
}

void client_game::update_late(void)
{
    mouse::update_late();

    if(client_game::vertical_sync)
        glfwSwapInterval(1);
    else glfwSwapInterval(0);
}

void client_game::render(void)
{
    const int scaled_width = globals::width / cxpr::max(1U, client_game::pixel_size);
    const int scaled_height = globals::height / cxpr::max(1U, client_game::pixel_size);

    glViewport(0, 0, scaled_width, scaled_height);
    glClearColor(globals::sky_color[0], globals::sky_color[1], globals::sky_color[2], globals::sky_color[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, globals::world_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    chunk_renderer::render();

    player_target::render();

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
        if(client_game::menu_background) {
            background::render();
        }
        else {
            glClearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }

    if(globals::gui_screen) {
        if(globals::registry.valid(globals::player)) {
            const float width_f = static_cast<float>(globals::width);
            const float height_f = static_cast<float>(globals::height);
            const ImU32 splash = ImGui::GetColorU32(ImVec4(0.00f, 0.00f, 0.00f, 0.75f));
            ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(), ImVec2(width_f, height_f), splash);
        }

        switch(globals::gui_screen) {
            case GUI_MAIN_MENU:
                main_menu::layout();
                break;
            case GUI_SERVER_LIST:
                server_list::layout();
                break;
            case GUI_SETTINGS:
                settings::layout();
                break;
            case GUI_PROGRESS:
                progress::layout();
                break;
        }
    }
    else if(debug_toggles::draw_debug_screen) {
        // This contains Minecraft-esque debug information
        // about the hardware, world state and other
        // things that might be uesful
        debug_screen::layout();
    }
}
