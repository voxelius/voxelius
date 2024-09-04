// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/shared/entity/chunk.hh>
#include <game/shared/event/chunk_create.hh>
#include <game/shared/event/chunk_remove.hh>
#include <game/shared/event/voxel_set.hh>
#include <game/shared/globals.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/voxel_coord.hh>
#include <game/shared/world.hh>

static emhash8::HashMap<ChunkCoord, Chunk *> chunks = {};

static void on_destroy_chunk(entt::registry &registry, entt::entity entity)
{
    ChunkComponent &component = registry.get<ChunkComponent>(entity);
    chunks.erase(component.coord);
    delete component.chunk;
}

void world::init(void)
{
    globals::registry.on_destroy<ChunkComponent>().connect<&on_destroy_chunk>();
}

Chunk *world::assign(const ChunkCoord &cpos, entt::entity entity)
{
    ChunkComponent &component = globals::registry.get_or_emplace<ChunkComponent>(entity);
    component.chunk = new Chunk();
    component.chunk->entity = entity;
    component.chunk->voxels.fill(NULL_VOXEL);
    component.coord = cpos;

    chunks.emplace(component.coord, component.chunk);

    ChunkCreateEvent event = {};
    event.chunk = component.chunk;
    event.coord = component.coord;

    globals::dispatcher.trigger(event);

    return component.chunk;
}

Chunk *world::find(const ChunkCoord &cpos)
{
    const auto it = chunks.find(cpos);
    if(it != chunks.cend())
        return it->second;
    return nullptr;
}

Chunk *world::find(entt::entity entity)
{
    if(ChunkComponent *component = globals::registry.try_get<ChunkComponent>(entity))
        return component->chunk;
    return nullptr;
}

Voxel world::get_voxel(const VoxelCoord &vpos)
{
    const auto cpos = VoxelCoord::to_chunk(vpos);
    const auto lpos = VoxelCoord::to_local(vpos);
    return world::get_voxel(cpos, lpos);
}

Voxel world::get_voxel(const ChunkCoord &cpos, const LocalCoord &lpos)
{
    const auto rvpos = ChunkCoord::to_voxel(cpos, lpos);
    const auto rcpos = VoxelCoord::to_chunk(rvpos);
    const auto rlpos = VoxelCoord::to_local(rvpos);
    const auto index = LocalCoord::to_index(rlpos);

    const auto it = chunks.find(rcpos);
    if(it != chunks.cend())
        return it->second->voxels[index];
    return NULL_VOXEL;
}

bool world::set_voxel(Voxel voxel, const VoxelCoord &vpos)
{
    const auto cpos = VoxelCoord::to_chunk(vpos);
    const auto lpos = VoxelCoord::to_local(vpos);
    return world::set_voxel(voxel, cpos, lpos);
}

bool world::set_voxel(Voxel voxel, const ChunkCoord &cpos, const LocalCoord &lpos)
{
    const auto rvpos = ChunkCoord::to_voxel(cpos, lpos);
    const auto rcpos = VoxelCoord::to_chunk(rvpos);
    const auto rlpos = VoxelCoord::to_local(rvpos);
    const auto index = LocalCoord::to_index(rlpos);

    if(Chunk *chunk = world::find(rcpos)) {
        chunk->voxels[index] = voxel;

        VoxelSetEvent event = {};
        event.cpos = rcpos;
        event.lpos = rlpos;
        event.vpos = rvpos;
        event.index = index;
        event.chunk = chunk;
        event.voxel = voxel;

        globals::dispatcher.trigger(event);

        return true;
    }

    return false;
}
