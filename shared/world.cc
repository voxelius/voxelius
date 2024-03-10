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
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/chunk_update.hh>
#include <shared/globals.hh>
#include <shared/world.hh>

static emhash8::HashMap<coord::chunk, Chunk *> chunks = {};

Chunk *world::create_chunk(const coord::chunk &cvec)
{
    const auto it = chunks.find(cvec);

    if(it == chunks.cend()) {
        auto *chunk = new Chunk{};
        chunk->entity = globals::registry.create();
        chunk->voxels.fill(NULL_VOXEL);
        chunk->lightmap.fill(NULL_LIGHT);

        auto &component = globals::registry.emplace<ChunkComponent>(chunk->entity);
        component.chunk = chunk;
        component.cvec = cvec;

        ChunkCreateEvent create_event = {};
        create_event.chunk = chunk;
        create_event.cvec = cvec;

        globals::dispatcher.trigger(create_event);

        return chunks.emplace(cvec, chunk).first->second;
    }

    return it->second;
}

Chunk *world::find_chunk(const coord::chunk &cvec)
{
    const auto it = chunks.find(cvec);
    if(it != chunks.cend())
        return it->second;
    return nullptr;
}

void world::remove_chunk(const coord::chunk &cvec)
{
    const auto it = chunks.find(cvec);

    if(it != chunks.cend()) {
        ChunkRemoveEvent remove_event = {};
        remove_event.chunk = it->second;
        remove_event.cvec = cvec;

        globals::dispatcher.trigger(remove_event);

        globals::registry.destroy(it->second->entity);

        delete it->second;

        chunks.erase(it);
    }
}

void world::purge_chunks(void)
{
    for(const auto it : chunks) {
        ChunkRemoveEvent remove_event = {};
        remove_event.chunk = it.second;
        remove_event.cvec = it.first;

        globals::dispatcher.trigger(remove_event);

        globals::registry.destroy(it.second->entity);

        delete it.second;
    }

    chunks.clear();
}

uint16_t world::get_voxel(const coord::voxel &vvec)
{
    const auto cvec = coord::to_chunk(vvec);
    const auto lvec = coord::to_local(vvec);
    const auto index = coord::to_index(lvec);
    const auto it = chunks.find(cvec);

    if(it != chunks.cend())
        return it->second->voxels[index];
    return NULL_VOXEL;
}

uint16_t world::get_voxel(const coord::chunk &cvec, const coord::local &lvec)
{
    const auto p_vvec = coord::to_voxel(cvec, lvec);
    const auto p_cvec = coord::to_chunk(p_vvec);
    const auto p_lvec = coord::to_local(p_vvec);
    const auto p_index = coord::to_index(p_lvec);
    const auto it = chunks.find(p_cvec);

    if(it != chunks.cend())
        return it->second->voxels[p_index];
    return NULL_VOXEL;
}

void world::set_voxel(uint16_t voxel, const coord::voxel &vvec)
{
    const auto cvec = coord::to_chunk(vvec);
    const auto lvec = coord::to_local(vvec);
    const auto index = coord::to_index(lvec);

    auto *chunk = world::create_chunk(cvec);
    chunk->voxels[index] = voxel;

    ChunkUpdateEvent update_event = {};
    update_event.chunk = chunk;
    update_event.voxel = voxel;
    update_event.cvec = cvec;
    update_event.lvec = lvec;
    update_event.vvec = vvec;
    update_event.index = index;

    globals::dispatcher.trigger(update_event);
}

void world::set_voxel(uint16_t voxel, const coord::chunk &cvec, const coord::local &lvec)
{
    const auto p_vvec = coord::to_voxel(cvec, lvec);
    const auto p_cvec = coord::to_chunk(p_vvec);
    const auto p_lvec = coord::to_local(p_vvec);
    const auto p_index = coord::to_index(p_lvec);

    auto *chunk = world::create_chunk(p_cvec);
    chunk->voxels[p_index] = voxel;

    ChunkUpdateEvent update_event = {};
    update_event.chunk = chunk;
    update_event.voxel = voxel;
    update_event.cvec = p_cvec;
    update_event.lvec = p_lvec;
    update_event.vvec = p_vvec;
    update_event.index = p_index;

    globals::dispatcher.trigger(update_event);
}
