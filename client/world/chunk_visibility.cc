// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/world/chunk_visibility.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <shared/world/chunk.hh>

void chunk_visibility::update(void)
{
    // Comparing squared distances is always faster than figuring square roots out
    const ChunkCoord::value_type view_distance = view::max_distance;
    const ChunkCoord::value_type view_distance_square = view_distance * view_distance;

    const auto view = globals::registry.view<ChunkComponent>();

    for(const auto [entity, chunk] : view.each()) {
        const ChunkCoord diff = chunk.coord - view::position.chunk;
        const ChunkCoord::value_type dist = diff[0] * diff[0] + diff[2] * diff[2];

        if(dist <= view_distance_square)
            globals::registry.emplace_or_replace<ChunkVisibleComponent>(entity);
        else globals::registry.remove<ChunkVisibleComponent>(entity);
    }
}
