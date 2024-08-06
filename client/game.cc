// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/player_look.hh>
#include <client/entity/player_move.hh>
#include <client/event/glfw_framebuffer_size.hh>
#include <client/gui/bitmap_font.hh>
#include <client/gui/text_builder.hh>
#include <client/gui/text_renderer.hh>
#include <client/gui/text_vbo.hh>
#include <client/input/keyboard.hh>
#include <client/input/keynames.hh>
#include <client/input/mouse.hh>
#include <client/world/chunk_mesher.hh>
#include <client/world/chunk_renderer.hh>
#include <client/world/chunk_visibility.hh>
#include <client/world/outline_renderer.hh>
#include <client/world/player_target.hh>
#include <client/world/voxel_anims.hh>
#include <client/world/voxel_atlas.hh>
#include <client/game.hh>
#include <client/globals.hh>
#include <client/screenshot.hh>
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

static BitmapFont font = {};
static TextVBO text = {};

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
}

void client_game::init(void)
{
    Config::add(globals::client_config, "game.vertical_sync", client_game::vertical_sync);
    Config::add(globals::client_config, "game.menu_background", client_game::menu_background);
    Config::add(globals::client_config, "game.username", client_game::username);
    Config::add(globals::client_config, "game.pixel_size", client_game::pixel_size);

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
    
    text_renderer::init();
    
    globals::dispatcher.sink<GlfwFramebufferSizeEvent>().connect<&on_glfw_framebuffer_size>();
}

void client_game::init_late(void)
{
    BitmapFont::load(font, "fonts/unifont.json");

    TextBuilder builder = {};
    builder.mode = TEXT_MODE_ASCII | TEXT_MODE_VT241 | TEXT_MODE_NOTCH;
    builder.background = Vec4f::transparent();
    builder.foreground = Vec4f::white();

    TextBuilder::append(builder, font, "This tests ASCII capabilities\n\tof the text renderer\n");
    TextBuilder::append(builder, font, "\n0123456789ABCDEF\n");
    TextBuilder::append(builder, font, "\033[30mTEST\033[31mTEST\033[32mTEST\033[33mTEST\033[34mTEST\033[35mTEST\033[36mTEST\033[37mTEST\033[38mTEST\033[39mTEST\n\033[40mTEST\033[41mTEST\033[42mTEST\033[43mTEST\033[44mTEST\033[45mTEST\033[46mTEST\033[47mTEST\033[48mTEST\033[49mTEST\n");
    TextBuilder::append(builder, font, "\u00A7c\u00A7lTEST\u00A7b\u00A7lTEST\u00A7a\u00A7lTEST\u00A7e\u00A7lTEST\u00A7d\u00A7lTEST\u00A7d\u00A7l\u00A7oTEST?\u00A7d\u00A7l\u00A7o\u00A7kABC\n");
    TextBuilder::append(builder, font, "When the \033[8mimposter\033[m is \033[92;8msus\n");
    TextBuilder::append(builder, font, "\033[1mBOLD\033[0m \033[4mUNDERLINE\033[0m \033[9mCROSSED\033[0m \033[1;3;4;9mOOPSIE\n");
    TextBuilder::append(builder, font, "проверка");
    TextVBO::create(text, builder);
}

void client_game::deinit(void)
{
    voxel_atlas::destroy();

    glDeleteRenderbuffers(1, &globals::world_fbo_depth);
    glDeleteTextures(1, &globals::world_fbo_color);
    glDeleteFramebuffers(1, &globals::world_fbo);

    TextVBO::destroy(text);
    BitmapFont::unload(font);

    text_renderer::deinit();

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

    const int hs = cxpr::floor<int>(static_cast<float>(globals::height) / 240.0f);
    const int ws = cxpr::floor<int>(static_cast<float>(globals::width) / 320.0f);
    const int scale = cxpr::min(hs, ws);

    text_renderer::prepare();
    text_renderer::draw(scale, scale, font, text, scale, true);
    text_renderer::draw(0, 0, font, text, scale, false);
}

void client_game::layout(void)
{
}
