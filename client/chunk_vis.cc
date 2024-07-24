// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/chunk_visible.hh>
#include <client/camera.hh>
#include <client/chunk_vis.hh>
#include <client/globals.hh>
#include <entt/entity/registry.hpp>
#include <shared/entity/chunk.hh>
#include <shared/const.hh>

void chunk_vis::update(void)
{
    const auto view = globals::registry.view<ChunkComponent>();

    // Comparing squared distances is always faster than figuring square roots out
    const ChunkPos::value_type view_distance = camera::view_distance * CHUNK_SIZE;
    const ChunkPos::value_type view_distance_square = view_distance * view_distance;
    const EntityPos &camera_pos = camera::position();

    for(const auto [entity, chunk] : view.each()) {
        const ChunkPos diff = chunk.coord - camera_pos.chunk;
        const ChunkPos::value_type dist = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if(dist <= view_distance_square)
            globals::registry.emplace_or_replace<ChunkVisibleComponent>(entity);
        else globals::registry.remove<ChunkVisibleComponent>(entity);
    }
}
