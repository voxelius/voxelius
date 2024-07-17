// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/entity/chunk.hh>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_fill.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/voxel_set.hh>
#include <shared/globals.hh>
#include <shared/world.hh>
#include <spdlog/spdlog.h>

static emhash8::HashMap<ChunkCoord, Chunk *> chunks = {};

Chunk *world::find_or_create_chunk(const ChunkCoord &cv)
{
    const auto it = chunks.find(cv);

    if(it != chunks.cend()) {
        // Just return the existing one
        return it->second;
    }

    Chunk *chunk = new Chunk();
    chunk->entity = globals::registry.create();
    voxel_storage::create(chunk->storage);

    auto &comp = globals::registry.emplace<ChunkComponent>(chunk->entity);
    comp.chunk = chunk;
    comp.coord = cv;

    chunks.emplace(cv, chunk);

    ChunkCreateEvent event = {};
    event.chunk = chunk;
    event.coord = cv;

    globals::dispatcher.trigger(event);

    return chunk;
}

Chunk *world::find_chunk(const ChunkCoord &cv)
{
    const auto it = chunks.find(cv);
    if(it != chunks.cend())
        return it->second;
    return nullptr;
}

void world::remove_chunk(const ChunkCoord &cv)
{
    const auto it = chunks.find(cv);

    if(it != chunks.cend()) {
        ChunkRemoveEvent event = {};
        event.chunk = it->second;
        event.coord = cv;

        globals::dispatcher.trigger(event);

        voxel_storage::destroy(it->second->storage);

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

        voxel_storage::destroy(it->second->storage);

        globals::registry.destroy(it->second->entity);

        delete it->second;
    }
}

Voxel world::get_voxel(const VoxelCoord &vv)
{
    const auto cv = coord::to_chunk(vv);
    const auto lv = coord::to_local(vv);
    return world::get_voxel(cv, lv);
}

Voxel world::get_voxel(const ChunkCoord &cv, const LocalCoord &lv)
{
    const auto rvv = coord::to_voxel(cv, lv);
    const auto rcv = coord::to_chunk(rvv);
    const auto rlv = coord::to_local(rvv);
    const auto idx = coord::to_index(rlv);

    const auto it = chunks.find(rcv);

    if(it == chunks.cend()) {
        // Just default to NULL_VOXEL
        return NULL_VOXEL;
    }

    return voxel_storage::get(it->second->storage, idx);
}

void world::set_voxel(Voxel voxel, const VoxelCoord &vv)
{
    const auto cv = coord::to_chunk(vv);
    const auto lv = coord::to_local(vv);
    world::set_voxel(voxel, cv, lv);
}

void world::set_voxel(Voxel voxel, const ChunkCoord &cv, const LocalCoord &lv)
{
    // Re-calculating chunk coordinates allows this
    // function to return valid results with invalid
    // local coordinates (for instance for when the voxel
    // update just so happens to touch a voxel in the immediate neighbour chunk)
    const auto rvv = coord::to_voxel(cv, lv);
    const auto rcv = coord::to_chunk(rvv);
    const auto rlv = coord::to_local(rvv);
    const auto idx = coord::to_index(rlv);

    auto *chunk = world::find_or_create_chunk(rcv);

    voxel_storage::set(chunk->storage, voxel, idx);

    VoxelSetEvent event = {};
    event.chunk = chunk;
    event.voxel = voxel;
    event.cv = rcv;
    event.lv = rlv;
    event.vv = rvv;

    globals::dispatcher.trigger(event);
}

void world::fill_chunks(Voxel voxel, const ChunkCoord &cvx, const ChunkCoord &cvy)
{
    for(ChunkCoord::value_type x = cvx.x; x <= cvy.x; ++x)
    for(ChunkCoord::value_type y = cvx.y; y <= cvy.y; ++y)
    for(ChunkCoord::value_type z = cvx.z; z <= cvy.z; ++z) {
        Chunk *chunk = world::find_or_create_chunk({x, y, z});

        voxel_storage::fill(chunk->storage, voxel);

        ChunkFillEvent event = {};
        event.coord = ChunkCoord(x, y, z);
        event.chunk = chunk;
        event.voxel = voxel;

        globals::dispatcher.trigger(event);
    }
}

void world::fill_voxels(Voxel voxel, const VoxelCoord &vvx, const VoxelCoord &vvy)
{
    spdlog::warn("world: fill_voxels is not implemented");
}
