// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/keyboard_key.hh>
#include <client/render/canvas.hh>
#include <client/render/font.hh>
#include <client/render/text.hh>
#include <client/ui/imgui.hh>
#include <client/ui/main_menu.hh>
#include <client/ui/screen.hh>
#include <client/globals.hh>

#include <client/render/voxel_anims.hh>
#include <client/render/voxel_atlas.hh>
#include <shared/entity/head.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/world/chunks.hh>
#include <shared/world/vdef.hh>
#include <spdlog/spdlog.h>

static imgui::Style style = {};
static Text title1 = {};
static Text title2 = {};
static Text button = {};

static void on_keyboard_key(const KeyboardKeyEvent &event)
{
    if(globals::registry.valid(globals::player)) {
        if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS) {
            switch(globals::ui_screen) {
                case ui::SCR_NONE:
                    globals::ui_screen = ui::SCR_MAIN_MENU;
                    break;
                case ui::SCR_MAIN_MENU:
                    globals::ui_screen = ui::SCR_NONE;
                    break;
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
        Chunk *chunk = chunks::create(cpos);
        chunk->voxels = voxels;
    }
}

static void very_stupid_stub()
{
    if(!globals::registry.valid(globals::player)) {
        vdef::purge();
        vdef::assign("stone", STONE);
        vdef::assign("slate", SLATE);
        vdef::assign("dirt", DIRT);
        vdef::assign("grass", GRASS);
        vdef::assign("vtest", VTEST);

        voxel_atlas::create(16, 16, vdef::textures.size());

        for(const vfs::path_t &path : vdef::textures) {
            if(!voxel_atlas::load(path)) {
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
        globals::player = globals::registry.create();
        globals::registry.emplace<PlayerComponent>(globals::player);
        globals::registry.emplace<HeadComponent>(globals::player);
        globals::registry.emplace<TransformComponent>(globals::player);
        globals::registry.emplace<VelocityComponent>(globals::player);

        globals::ui_screen = ui::SCR_NONE;
    }
}

void main_menu::init()
{
    style.button_background[imgui::BUTTON_IDLE] = COL_TRANSPARENT;
    style.button_background[imgui::BUTTON_HOVER] = vector4_t{1.0, 1.0, 1.0, 0.125};
    style.button_background[imgui::BUTTON_PRESS] = vector4_t{1.0, 1.0, 1.0, 0.250};
    style.button_foreground[imgui::BUTTON_IDLE] = COL_LIGHT_GRAY;
    style.button_foreground[imgui::BUTTON_HOVER] = COL_WHITE;
    style.button_foreground[imgui::BUTTON_PRESS] = COL_WHITE;
    style.button_margin = vector2i_t{4, 4};

    title1.create(32, 1);
    title2.create(32, 1);
    button.create(64, 1);

    title1.set(0, L"Voxelius");
    title2.set(0, L"Indev 0.0.1");

    globals::dispatcher.sink<KeyboardKeyEvent>().connect<&on_keyboard_key>();
}

void main_menu::deinit()
{
    button.destroy();
    title2.destroy();
    title1.destroy();
}

void main_menu::draw_ui()
{
    const int tstep1 = globals::pc_vga_8x16.get_glyph_height() * 2U;
    const int tstep2 = globals::pc_vga_8x8.get_glyph_height() * 2U + 4U;
    const int bstep = 2 * style.button_margin.y + globals::pc_vga_8x8.get_glyph_height();
    vector2i_t pos = {globals::window_width * 0.03125, globals::window_height * 0.10};

    canvas::text(globals::ui_scale * pos.x, globals::ui_scale * pos.y, title1, globals::pc_vga_8x16, globals::ui_scale * 2U);
    pos.y += tstep1;

    canvas::text(globals::ui_scale * pos.x, globals::ui_scale * pos.y, title2, globals::pc_vga_8x8, globals::ui_scale);
    pos.y += tstep2;

    if(globals::registry.valid(globals::player)) {
        button.set(0, L"Back to Game");
        if(imgui::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
            globals::ui_screen = ui::SCR_NONE;
        pos.y += bstep;

        button.set(0, L"Disconnect");
        if(imgui::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
            spdlog::info("We can't disconnect yet!");
        pos.y += bstep * 1.5;
    }
    else {
        button.set(0, L"DEBUG SESSION");
        if(imgui::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
            very_stupid_stub();
        pos.y += bstep * 1.5;
    }

    button.set(0, L"Join a Server");
    if(imgui::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
        globals::ui_screen = ui::SCR_SERVER_BROWSER;
    pos.y += bstep;

    button.set(0, L"Settings");
    if(imgui::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
        globals::ui_screen = ui::SCR_SETTINGS;
    pos.y += bstep;

    button.set(0, L"Quit");
    if(imgui::button(pos.x, pos.y, button, globals::pc_vga_8x8, style))
        glfwSetWindowShouldClose(globals::window, true);
    pos.y += bstep;
}
