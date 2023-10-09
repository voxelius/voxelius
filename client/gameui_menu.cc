// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gameui_menu.hh>
#include <client/glfw_events.hh>
#include <client/globals.hh>
#include <client/ui_color.hh>
#include <client/ui_draw.hh>
#include <client/ui_immediate.hh>
#include <client/ui_font.hh>
#include <client/ui_text.hh>

#include <client/atlas.hh>
#include <client/voxel_anims.hh>
#include <shared/head.hh>
#include <shared/player.hh>
#include <shared/transform.hh>
#include <shared/world.hh>
#include <shared/velocity.hh>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>

static ui::im::Style style = {};
static ui::Text title1 = {};
static ui::Text title2 = {};
static ui::Text button = {};

static void on_key(const KeyEvent &event)
{
    if(globals::world.registry.valid(globals::player)) {
        if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
            if(!globals::gameui_visible) {
                globals::gameui_screen = GameUiScreen::MainMenu;
                globals::gameui_visible = true;
            }
            else if(globals::gameui_screen == GameUiScreen::MainMenu) {
                globals::gameui_screen = GameUiScreen::Undefined;
                globals::gameui_visible = false;
            }
        }
    }
}

constexpr static const uint32_t STONE   = 1;
constexpr static const uint32_t SLATE   = 2;
constexpr static const uint32_t DIRT    = 3;
constexpr static const uint32_t GRASS   = 4;
constexpr static const uint32_t VTEST   = 5;

// Surface level for world generation
constexpr static const int64_t SURFACE = 0;

#include <glm/gtc/noise.hpp>

static voxel_t voxel_at(const voxel_pos_t &vpos)
{
    int64_t surf = SURFACE + 8.0 * glm::simplex(vector2_t{vpos.x, vpos.z} / 48.0);
    if(vpos.y <= surf - 8)
        return make_voxel(STONE, NULL_VOXEL_STATE);
    if(vpos.y <= surf - 1)
        return make_voxel(DIRT, NULL_VOXEL_STATE);
    if(vpos.y <= surf)
        return make_voxel(GRASS, NULL_VOXEL_STATE);
    return NULL_VOXEL;
}

static void generate(const chunk_pos_t &cpos)
{
    spdlog::trace("generating {} {} {}", cpos.x, cpos.y, cpos.z);

    bool empty = true;
    voxel_array_t voxels = {};

    for(size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = coord::to_local(i);
        const auto vpos = coord::to_voxel(cpos, lpos);
        const auto voxel = voxel_at(vpos);
        if(voxel != NULL_VOXEL)
            empty = false;
        voxels[i] = voxel;
    }

    if(!empty) {
        Chunk *chunk = globals::world.create_chunk(cpos);
        chunk->voxels = voxels;
    }
}

static void very_stupid_stub()
{
    if(!globals::world.registry.valid(globals::player)) {
        vdef::purge();
        vdef::assign("stone", STONE);
        vdef::assign("slate", SLATE);
        vdef::assign("dirt", DIRT);
        vdef::assign("grass", GRASS);
        vdef::assign("vtest", VTEST);

        atlas::create(16, 16, vdef::textures.size());

        for(const vfs::path_t &path : vdef::textures) {
            if(!atlas::load(path)) {
                spdlog::critical("atlas: {}: load failed", path.string());
                std::terminate();
            }
        }

        voxel_anims::construct();

        for(int x = -8; x < 7; ++x) {
            for(int z = -8; z < 7; ++z) {
                for(int y = -1; y < 2; ++y) {
                    generate(chunk_pos_t{x, y, z});
                }
            }
        }

        spdlog::info("spawning local player");
        globals::player = globals::world.registry.create();
        globals::world.registry.emplace<PlayerComponent>(globals::player);
        globals::world.registry.emplace<HeadComponent>(globals::player);
        globals::world.registry.emplace<TransformComponent>(globals::player);
        globals::world.registry.emplace<VelocityComponent>(globals::player);

        globals::gameui_screen = GameUiScreen::Undefined;
        globals::gameui_visible = false;
    }
}

void gameui_menu::init()
{
    style.button_background[ui::im::BUTTON_IDLE] = ui::TRANSPARENT;
    style.button_background[ui::im::BUTTON_HOVER] = vector4_t{1.0, 1.0, 1.0, 0.125};
    style.button_background[ui::im::BUTTON_PRESS] = vector4_t{1.0, 1.0, 1.0, 0.250};
    style.button_foreground[ui::im::BUTTON_IDLE] = ui::LIGHT_GRAY;
    style.button_foreground[ui::im::BUTTON_HOVER] = ui::WHITE;
    style.button_foreground[ui::im::BUTTON_PRESS] = ui::WHITE;
    style.button_margin = vector2i_t{4, 4};

    title1.create(32, 1);
    title2.create(32, 1);
    button.create(64, 1);

    title1.set(0, L"Voxelius");
    title2.set(0, L"Indev 0.0.1");

    globals::dispatcher.sink<KeyEvent>().connect<&on_key>();
}

void gameui_menu::deinit()
{
    button.destroy();
    title2.destroy();
    title1.destroy();
}

void gameui_menu::draw_ui()
{
    style.element_scale = globals::gameui_scale;

    const int tstep1 = globals::pc_vga_8x16.get_glyph_height() * 2U;
    const int tstep2 = globals::pc_vga_8x8.get_glyph_height() * 2U + 4U;
    const int bstep = 2 * style.button_margin.y + globals::pc_vga_8x8.get_glyph_height();
    vector2i_t pos = {32, 32};

    ui::draw::text(pos.x / 2, pos.y / 2, title1, globals::pc_vga_8x16, style.element_scale * 2U);
    pos.y += tstep1;

    ui::draw::text(pos.x, pos.y, title2, globals::pc_vga_8x8, style.element_scale);
    pos.y += tstep2;

    button.set(0, L"DEBUG SESSION");
    if(ui::im::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
        very_stupid_stub();
    pos.y += bstep;

    button.set(0, L"Join a Server");
    if(ui::im::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
        globals::gameui_screen = GameUiScreen::ServerBrowser;
    pos.y += bstep;

    button.set(0, L"Settings");
    if(ui::im::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
        globals::gameui_screen = GameUiScreen::Settings;
    pos.y += bstep;

    button.set(0, L"Quit");
    if(ui::im::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
        glfwSetWindowShouldClose(globals::window, true);
    pos.y += bstep;
}
