// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <client/debug_session.hh>
#include <client/globals.hh>
#include <client/ui_screen.hh>
#include <client/voxel_anims.hh>
#include <client/voxel_atlas.hh>
#include <entt/entity/registry.hpp>
#include <glm/gtc/noise.hpp>
#include <shared/entity/head.hh>
#include <shared/entity/player.hh>
#include <shared/entity/transform.hh>
#include <shared/entity/velocity.hh>
#include <shared/vdef.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

constexpr static const uint32_t STONE   = 1;
constexpr static const uint32_t SLATE   = 2;
constexpr static const uint32_t DIRT    = 3;
constexpr static const uint32_t GRASS   = 4;
constexpr static const uint32_t VTEST   = 5;

// Surface level for world generation
constexpr static const int64_t SURFACE = 0;


static voxel_t voxel_at(const voxel_pos_t &vpos)
{
    int64_t surf = SURFACE + 8.0 * glm::simplex(glm::dvec2{vpos.x, vpos.z} / 48.0);
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
    Chunk::array_type voxels = {};

    for(size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = coord::to_local(i);
        const auto vpos = coord::to_voxel(cpos, lpos);
        const auto voxel = voxel_at(vpos);
        if(voxel != NULL_VOXEL)
            empty = false;
        voxels[i] = voxel;
    }

    if(!empty) {
        Chunk *chunk = world::create_chunk(cpos);
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

        unsigned int w = 0U;
        for(int x = -8; x < 7; ++x) {
            for(int z = -8; z < 7; ++z) {
                for(int y = -2; y < 2; ++y) {
                    generate({x, y, z});
                    //Chunk *chunk = world::create_chunk(chunk_pos_t{x, y, z});
                    //chunk->voxels.fill(make_voxel(cxmath::clamp(x * z, -1, 1) + 2, NULL_VOXEL_STATE));
                }
            }
        }

        spdlog::info("spawning local player");
        globals::player = globals::registry.create();
        globals::registry.emplace<PlayerComponent>(globals::player);
        globals::registry.emplace<VelocityComponent>(globals::player);

        auto &head = globals::registry.emplace<HeadComponent>(globals::player);
        head.offset = glm::dvec3{0.4, 1.4, 0.4};

        auto &transform = globals::registry.emplace<TransformComponent>(globals::player);
        transform.position.y += 16.0;

        globals::ui_screen = ui::SCREEN_NONE;
    }
}
