// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/globals.hh>
#include <shared/world.hh>

Chunk *World::create_chunk(const chunk_pos_t &cpos)
{
    Chunk *chunk = find_chunk(cpos);

    if(chunk == nullptr) {
        chunk = new Chunk{};
        chunk->voxels.fill(NULL_VOXEL);
        chunk->entity = registry.create();

        ChunkComponent &component = registry.emplace<ChunkComponent>(chunk->entity);
        component.chunk = chunk;
        component.cpos = cpos;

        chunks.emplace(cpos, chunk);

        globals::dispatcher.trigger(ChunkCreateEvent {
            .world = this,
            .chunk = chunk,
            .cpos = cpos,
        });
    }

    return chunk;
}

Chunk *World::find_chunk(const chunk_pos_t &cpos) const
{
    if(const auto it = chunks.find(cpos); it != chunks.cend())
        return it->second;
    return nullptr;
}

void World::remove_chunk(const chunk_pos_t &cpos)
{
    if(const auto it = chunks.find(cpos); it != chunks.cend()) {
        globals::dispatcher.enqueue(ChunkRemoveEvent {
            .world = this,
            .chunk = it->second,
            .cpos = cpos,
        });

        registry.destroy(it->second->entity);
        delete it->second;
        chunks.erase(it);
    }
}

voxel_t World::get_voxel(const voxel_pos_t &vpos) const
{
    return get_voxel(coord::to_chunk(vpos), coord::to_local(vpos));
}

voxel_t World::get_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos) const
{
    if(const auto it = chunks.find(cpos); it != chunks.cend())
        return it->second->voxels.at(coord::to_index(lpos));
    return NULL_VOXEL;
}

void World::set_voxel(voxel_t voxel, const voxel_pos_t &vpos)
{
    const auto cpos = coord::to_chunk(vpos);
    const auto lpos = coord::to_local(vpos);
    const auto index = coord::to_index(lpos);

    Chunk *chunk = create_chunk(cpos);
    chunk->voxels[index] = voxel;

    globals::dispatcher.enqueue(VoxelUpdateEvent {
        .world = this,
        .chunk = chunk,
        .voxel = voxel,
        .cpos = cpos,
        .lpos = lpos,
        .vpos = vpos,
        .index = index,
    });
}

void World::set_voxel(voxel_t voxel, const chunk_pos_t &cpos, const local_pos_t &lpos)
{
    const auto p_vpos = coord::to_voxel(cpos, lpos);
    const auto p_cpos = coord::to_chunk(p_vpos);
    const auto p_lpos = coord::to_local(p_vpos);
    const auto p_index = coord::to_index(p_lpos);

    Chunk *chunk = create_chunk(p_cpos);
    chunk->voxels[p_index] = voxel;

    globals::dispatcher.enqueue(VoxelUpdateEvent {
        .world = this,
        .chunk = chunk,
        .voxel = voxel,
        .cpos = p_cpos,
        .lpos = p_lpos,
        .vpos = p_vpos,
        .index = p_index,
    });
}

void World::purge()
{
    auto it = chunks.begin();

    while(it != chunks.end()) {
        globals::dispatcher.enqueue(ChunkRemoveEvent {
            .world = this,
            .chunk = it->second,
            .cpos = it->first,
        });

        registry.destroy(it->second->entity);
        delete it->second;
        it = chunks.erase(it);
    }

    registry.clear();
    chunks.clear();
}
