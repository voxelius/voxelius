// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/entity/chunk_visible.hh>
#include <client/chunk_vis.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <shared/entity/chunk.hh>
#include <shared/const.hh>

void chunk_vis::update(void)
{
    // Comparing squared distances is always faster than figuring square roots out
    const ChunkPos::value_type view_distance = view::max_distance;
    const ChunkPos::value_type view_distance_square = view_distance * view_distance;

    const auto view = globals::registry.view<ChunkComponent>();

    for(const auto [entity, chunk] : view.each()) {
        const ChunkPos diff = chunk.coord - view::position.chunk;
        const ChunkPos::value_type dist = diff.x * diff.x + diff.z * diff.z;

        if(dist <= view_distance_square)
            globals::registry.emplace_or_replace<ChunkVisibleComponent>(entity);
        else globals::registry.remove<ChunkVisibleComponent>(entity);
    }
}
