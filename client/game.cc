// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/canvas/canvas.hh>
#include <client/canvas/font.hh>
#include <client/debug/debug_screen.hh>
#include <client/debug/debug_session.hh>
#include <client/debug/debug_toggles.hh>
#include <client/entity/player_look.hh>
#include <client/entity/player_move.hh>
#include <client/event/glfw_framebuffer_size.hh>
#include <client/gui/screen.hh>
#include <client/world/chunk_mesher.hh>
#include <client/world/chunk_renderer.hh>
#include <client/world/chunk_visibility.hh>
#include <client/world/player_target.hh>
#include <client/world/voxel_anims.hh>
#include <client/world/voxel_atlas.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/keyboard.hh>
#include <client/keynames.hh>
#include <client/mouse.hh>
#include <client/screenshot.hh>
#include <client/vdraw.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <GLFW/glfw3.h>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/util/physfs.hh>
#include <shared/world/ray_dda.hh>
#include <shared/world/world.hh>
#include <shared/config.hh>
#include <spdlog/spdlog.h>

bool client_game::vertical_sync = true;
bool client_game::menu_background = true;
std::string client_game::username = "player";
unsigned int client_game::pixel_size = 4U;

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

#if !defined(_WIN32)
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::critical("opengl: world framebuffer is incomplete");
        glDeleteRenderbuffers(1, &globals::world_fbo_depth);
        glDeleteTextures(1, &globals::world_fbo_color);
        glDeleteFramebuffers(1, &globals::world_fbo);
        std::terminate();
    }
#endif

    constexpr float width_base = 320.0f;
    constexpr float height_base = 240.0f;
    const float width_float = event.width;
    const float height_float = event.height;
    const unsigned int wscale = cxpr::max(1U, cxpr::floor<unsigned int>(width_float / width_base));
    const unsigned int hscale = cxpr::max(1U, cxpr::floor<unsigned int>(height_float / height_base));
    globals::gui_scale = cxpr::min(wscale, hscale);
}

void client_game::init(void)
{
    Config::add(globals::client_config, "game.vertical_sync", client_game::vertical_sync);
    Config::add(globals::client_config, "game.menu_background", client_game::menu_background);
    Config::add(globals::client_config, "game.username", client_game::username);
    Config::add(globals::client_config, "game.pixel_size", client_game::pixel_size);

    //settings::add_checkbox(5, settings::VIDEO, "game.vertical_sync", client_game::vertical_sync, false);
    //settings::add_checkbox(0, settings::VIDEO_GUI, "game.menu_background", client_game::menu_background, true);
    //settings::add_input(1, settings::GENERAL, "game.username", client_game::username, false, false);
    //settings::add_slider(1, settings::VIDEO, "game.pixel_size", client_game::pixel_size, 1U, 4U, true);

    //language::init();

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

    canvas::init();

    if(!Font::load(globals::font_8px, 8, 8, "fonts/cga_8x8.png"))
    if(!Font::load(globals::font_8px, 8, 8, "fonts/msx_8x8.png")) {
        spdlog::critical("font_8px: unable to locate a valid font atlas");
        std::terminate();
    }

    if(!Font::load(globals::font_16px, 8, 16, "fonts/unscii_8x16.png"))
    if(!Font::load(globals::font_16px, 8, 16, "fonts/unifont_8x16.png"))
    if(!Font::load(globals::font_16px, 8, 16, "fonts/vga_8x16.png")) {
        spdlog::critical("font_16px: unable to locate a valid font atlas");
        std::terminate();
    }

    vdraw::init();

    debug_toggles::init();
    debug_screen::init();
    debug_session::init();

    // main_menu::init();
    // progress::init();
    // server_list::init();
    // settings::init();

    globals::gui_keybind_ptr = nullptr;
    globals::gui_scale = 0U;
    globals::gui_screen = GUI_MAIN_MENU;

    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
}

void client_game::init_late(void)
{
    debug_session::run();

    // language::init_late();

    // settings::init_late();
}

void client_game::deinit(void)
{
    debug_screen::deinit();

    voxel_atlas::destroy();

    glDeleteRenderbuffers(1, &globals::world_fbo_depth);
    glDeleteTextures(1, &globals::world_fbo_color);
    glDeleteFramebuffers(1, &globals::world_fbo);

    // background::deinit();

    vdraw::deinit();

    canvas::deinit();

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
    glClearColor(0.529f, 0.808f, 0.922f, 1.000f);
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

void client_game::draw_gui(void)
{
#if 0
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
            const ImU32 splash = ImGui::GetColorU32(ImVec4(0.000f, 0.000f, 0.000f, 0.900f));
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
        debug_screen::draw_gui();
    }
#else
    debug_screen::draw_gui();
#endif
}
