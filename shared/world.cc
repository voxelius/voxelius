// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/entity/chunk.hh>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/voxel_set.hh>
#include <shared/coord.hh>
#include <shared/globals.hh>
#include <shared/world.hh>

static emhash8::HashMap<ChunkPos, Chunk *> chunks = {};

Chunk *world::find_or_create_chunk(const ChunkPos &cpos)
{
    const auto it = chunks.find(cpos);
    if(it != chunks.cend())
        return it->second;


    Chunk *chunk = new Chunk();
    Chunk::create_storage(chunk[0]);
    chunk->entity = globals::registry.create();

    auto &component = globals::registry.emplace<ChunkComponent>(chunk->entity);
    component.chunk = chunk;
    component.coord = cpos;

    chunks.emplace(cpos, chunk);

    ChunkCreateEvent event = {};
    event.chunk = chunk;
    event.coord = cpos;

    globals::dispatcher.trigger(event);

    return chunk;
}

Chunk *world::find_chunk(const ChunkPos &cpos)
{
    const auto it = chunks.find(cpos);

    if(it != chunks.cend())
        return it->second;
    return nullptr;
}

void world::remove_chunk(const ChunkPos &cpos)
{
    const auto it = chunks.find(cpos);

    if(it != chunks.cend()) {
        ChunkRemoveEvent event = {};
        event.chunk = it->second;
        event.coord = it->first;

        globals::dispatcher.trigger(event);

        globals::registry.destroy(it->second->entity);

        delete it->second;

        chunks.erase(it);
    }
}

void world::purge_chunks(void)
{
    for(auto it = chunks.begin(); it != chunks.end(); it = chunks.erase(it)) {
        ChunkRemoveEvent event = {};
        event.chunk = it->second;
        event.coord = it->first;

        globals::dispatcher.trigger(event);

        globals::registry.destroy(it->second->entity);

        delete it->second;
    }
}

Voxel world::get_voxel(const VoxelPos &vpos)
{
    const auto cpos = coord::to_chunk(vpos);
    const auto lpos = coord::to_local(vpos);
    return world::get_voxel(cpos, lpos);
}

Voxel world::get_voxel(const ChunkPos &cpos, const LocalPos &lpos)
{
    const auto rvpos = coord::to_voxel(cpos, lpos);
    const auto rcpos = coord::to_chunk(rvpos);
    const auto rlpos = coord::to_local(rvpos);
    const auto index = coord::to_index(rlpos);

    const auto it = chunks.find(rcpos);

    if(it != chunks.cend())
        return Chunk::get_voxel(it->second[0], index);
    return NULL_VOXEL;
}

void world::set_voxel(Voxel voxel, const VoxelPos &vpos)
{
    const auto cpos = coord::to_chunk(vpos);
    const auto lpos = coord::to_local(vpos);
    world::set_voxel(voxel, cpos, lpos);
}

void world::set_voxel(Voxel voxel, const ChunkPos &cpos, const LocalPos &lpos)
{
    const auto rvpos = coord::to_voxel(cpos, lpos);
    const auto rcpos = coord::to_chunk(rvpos);
    const auto rlpos = coord::to_local(rvpos);
    const auto index = coord::to_index(rlpos);

    Chunk *chunk = world::find_or_create_chunk(rcpos);

    Chunk::set_voxel(chunk[0], voxel, index);

    VoxelSetEvent event = {};
    event.cpos = rcpos;
    event.lpos = rlpos;
    event.vpos = rvpos;
    event.index = index;
    event.chunk = chunk;
    event.voxel = voxel;

    globals::dispatcher.trigger(event);
}
