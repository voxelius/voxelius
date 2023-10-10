// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/debug_session.hh>
#include <client/globals.hh>
#include <client/ui_screen.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <shared/chunks.hh>
#include <shared/entity/head.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/vdef.hh>
#include <spdlog/spdlog.h>

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

void debug_session::run()
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

        globals::ui_screen = ui::SCREEN_NONE;
    }
}
