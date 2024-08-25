// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <game/client/chunk_visibility.hh>
#include <game/client/globals.hh>
#include <game/client/view.hh>
#include <game/shared/chunk.hh>
#include <game/shared/world.hh>

static ChunkCoord cached_cpos = {};
static unsigned int cached_dist = {};

static bool is_visible(const ChunkCoord &cvec)
{
    const auto dx = cxpr::abs(cvec[0] - cached_cpos[0]);
    const auto dy = cxpr::abs(cvec[2] - cached_cpos[2]);

    if((dx <= cached_dist) && (dy <= cached_dist))
        return true;
    return false;
}

void chunk_visibility::update_chunk(entt::entity entity)
{
    if(const ChunkComponent *chunk = globals::registry.try_get<ChunkComponent>(entity)) {
        if(is_visible(chunk->coord))
            globals::registry.emplace_or_replace<ChunkVisibleComponent>(entity);
        else globals::registry.remove<ChunkVisibleComponent>(entity);
    }
}

void chunk_visibility::update_chunk(const ChunkCoord &cvec)
{
    if(const Chunk *chunk = world::find_chunk(cvec)) {
        if(is_visible(cvec))
            globals::registry.emplace_or_replace<ChunkVisibleComponent>(chunk->entity);
        else globals::registry.remove<ChunkVisibleComponent>(chunk->entity);
    }
}

void chunk_visibility::update_chunks(void)
{
    const auto view = globals::registry.view<ChunkComponent>();
    
    for(const auto [entity, chunk] : view.each()) {
        if(is_visible(chunk.coord))
            globals::registry.emplace_or_replace<ChunkVisibleComponent>(entity);
        else globals::registry.remove<ChunkVisibleComponent>(entity);
    }
}

void chunk_visibility::update(void)
{
    if(globals::registry.valid(globals::player)) {
        if((cached_cpos != view::position.chunk) || (cached_dist != view::max_distance)) {
            cached_cpos = view::position.chunk;
            cached_dist = view::max_distance;
            chunk_visibility::update_chunks();
            return;
        }
        
        cached_cpos = view::position.chunk;
        cached_dist = view::max_distance;
        return;
    }
    
    cached_cpos = view::position.chunk + 1;
    cached_dist = view::max_distance + 1;
}
