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
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/chunk_update.hh>
#include <shared/globals.hh>
#include <shared/world.hh>

Chunk *World::create(World &world, const coord::chunk &cc)
{
    Chunk *chunk = World::find(world, cc);

    if(chunk == nullptr) {
        chunk = new Chunk{};
        chunk->entity = world.registry.create();
        chunk->voxels.fill(NULL_VOXEL);
        chunk->lightmap.fill(NULL_LIGHT);

        auto &component = world.registry.emplace<ChunkComponent>(chunk->entity);
        component.chunk = chunk;
        component.position = cc;

        world.chunks.emplace(cc, chunk);

        ChunkCreateEvent event = {};
        event.chunk = chunk;
        event.world = &world;
        event.position = cc;

        globals::dispatcher.trigger(event);
    }

    return chunk;
}

Chunk *World::find(World &world, const coord::chunk &cc)
{
    if(const auto it = world.chunks.find(cc); it != world.chunks.cend())
        return it->second;
    return nullptr;
}

void World::remove(World &world, const coord::chunk &cc)
{
    if(const auto it = world.chunks.find(cc); it != world.chunks.cend()) {
        ChunkRemoveEvent event = {};
        event.chunk = it->second;
        event.world = &world;
        event.position = cc;

        globals::dispatcher.trigger(event);

        world.registry.destroy(it->second->entity);

        delete it->second;

        world.chunks.erase(it);
    }
}

void World::purge(World &world)
{
    auto it = world.chunks.begin();

    while(it != world.chunks.end()) {
        ChunkRemoveEvent event = {};
        event.chunk = it->second;
        event.world = &world;
        event.position = it->first;

        globals::dispatcher.trigger(event);

        world.registry.destroy(it->second->entity);

        delete it->second;

        it = world.chunks.erase(it);
    }

    world.chunks.clear();
    world.registry.clear();
}

uint16_t World::get(World &world, const coord::voxel &vv)
{
    const auto cc = coord::to_chunk(vv);
    const auto ll = coord::to_local(vv);
    const auto index = coord::to_index(ll);
    if(const auto it = world.chunks.find(cc); it != world.chunks.cend())
        return it->second->voxels[index];
    return NULL_VOXEL;
}

uint16_t World::get(World &world, const coord::chunk &cc, const coord::local &ll)
{
    const auto pvv = coord::to_voxel(cc, ll);
    const auto pcc = coord::to_chunk(pvv);
    const auto pll = coord::to_local(pvv);
    const auto index = coord::to_index(pll);
    if(const auto it = world.chunks.find(pcc); it != world.chunks.cend())
        return it->second->voxels[index];
    return NULL_VOXEL;
}

void World::set(World &world, uint16_t voxel, const coord::voxel &vv)
{
    const auto cc = coord::to_chunk(vv);
    const auto ll = coord::to_local(vv);
    const auto index = coord::to_index(ll);

    Chunk *chunk = World::create(world, cc);
    chunk->voxels[index] = voxel;

    ChunkUpdateEvent event = {};
    event.chunk = chunk;
    event.world = &world;
    event.voxel = voxel;
    event.position_chunk = cc;
    event.position_local = ll;
    event.position_voxel = vv;
    event.index = index;

    globals::dispatcher.trigger(event);
}

void World::set(World &world, uint16_t voxel, const coord::chunk &cc, const coord::local &ll)
{
    const auto pvv = coord::to_voxel(cc, ll);
    const auto pcc = coord::to_chunk(pvv);
    const auto pll = coord::to_local(pvv);
    const auto index = coord::to_index(pll);

    Chunk *chunk = World::create(world, pcc);
    chunk->voxels[index] = voxel;

    ChunkUpdateEvent event = {};
    event.chunk = chunk;
    event.world = &world;
    event.voxel = voxel;
    event.position_chunk = pcc;
    event.position_local = pll;
    event.position_voxel = pvv;
    event.index = index;

    globals::dispatcher.trigger(event);
}
