// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/chunk_visibility.hh>
#include <client/globals.hh>
#include <client/view.hh>
#include <entt/entity/registry.hpp>
#include <shared/chunk.hh>

static ChunkCoord prev = {};

static void do_update(void)
{
    // Comparing squared distances is always faster than figuring square roots out
    const ChunkCoord::value_type view_distance = view::max_distance;
    const ChunkCoord::value_type view_distance_square = view_distance * view_distance;

    const auto view = globals::registry.view<ChunkComponent>();

    for(const auto [entity, chunk] : view.each()) {
        const ChunkCoord::value_type dx = cxpr::abs(chunk.coord[0] - view::position.chunk[0]);
        const ChunkCoord::value_type dz = cxpr::abs(chunk.coord[2] - view::position.chunk[2]);
        const ChunkCoord::value_type maxdist = view::max_distance;
        
        if((dx <= maxdist) && (dz <= maxdist)) {
            globals::registry.emplace_or_replace<ChunkVisibleComponent>(entity);
            continue;
        }
        
        // Chunk is not visible, remove the component
        else globals::registry.remove<ChunkVisibleComponent>(entity);
    }
}


void chunk_visibility::update(void)
{
    if(globals::registry.valid(globals::player)) {
        if(prev != view::position.chunk)
            do_update();
        prev = view::position.chunk;
    }
    else {
        prev = view::position.chunk + 1;
    }
}
