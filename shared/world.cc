// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/globals.hh>
#include <shared/world.hh>

Chunk *World::create_chunk(const chunk_coord_t &cvec)
{
    Chunk *chunk = find_chunk(cvec);

    if(chunk == nullptr) {
        chunk = new Chunk{};
        chunk->voxels.fill(NULL_VOXEL);
        chunk->entity = registry.create();
        chunk->ref_count = 0;

        ChunkComponent &component = registry.emplace<ChunkComponent>(chunk->entity);
        component.chunk = chunk;
        component.cvec = cvec;

        chunks.emplace(cvec, chunk);

        globals::dispatcher.trigger(ChunkCreateEvent {
            .world = this,
            .chunk = chunk,
            .cvec = cvec,
        });
    }

    return chunk;
}

Chunk *World::find_chunk(const chunk_coord_t &cvec) const
{
    if(const auto it = chunks.find(cvec); it != chunks.cend())
        return it->second;
    return nullptr;
}

void World::remove_chunk(const chunk_coord_t &cvec)
{
    if(const auto it = chunks.find(cvec); it != chunks.cend()) {
        globals::dispatcher.trigger(ChunkRemoveEvent {
            .world = this,
            .chunk = it->second,
            .cvec = cvec,
        });

        registry.destroy(it->second->entity);
        delete it->second;
        chunks.erase(it);
    }
}

voxel_t World::get_voxel(const voxel_coord_t &vvec) const
{
    return get_voxel(coord::to_chunk(vvec), coord::to_local(vvec));
}

voxel_t World::get_voxel(const chunk_coord_t &cvec, const local_coord_t &lvec) const
{
    if(const auto it = chunks.find(cvec); it != chunks.cend())
        return it->second->voxels.at(coord::to_index(lvec));
    return NULL_VOXEL;
}

void World::set_voxel(voxel_t voxel, const voxel_coord_t &vvec)
{
    const chunk_coord_t cvec = coord::to_chunk(vvec);
    const local_coord_t lvec = coord::to_voxel(vvec);
    const size_t index = coord::to_index(lvec);

    Chunk *chunk = create_chunk(cvec);
    chunk->voxels[index] = voxel;

    globals::dispatcher.trigger(VoxelUpdateEvent {
        .world = this,
        .chunk = chunk,
        .voxel = voxel,
        .cvec = cvec,
        .lvec = lvec,
        .vvec = vvec,
        .index = index,
    });
}

void World::set_voxel(voxel_t voxel, const chunk_coord_t &cvec, const local_coord_t &lvec)
{
    const voxel_coord_t vvec = coord::to_voxel(cvec, lvec);
    const chunk_coord_t pcvec = coord::to_chunk(vvec);
    const local_coord_t plvec = coord::to_local(vvec);
    const size_t index = coord::to_index(plvec);

    Chunk *chunk = create_chunk(pcvec);
    chunk->voxels[index] = voxel;

    globals::dispatcher.trigger(VoxelUpdateEvent {
        .world = this,
        .chunk = chunk,
        .voxel = voxel,
        .cvec = pcvec,
        .lvec = plvec,
        .vvec = vvec,
        .index = index,
    });
}

void World::purge()
{
    for(const auto it : chunks)
        remove_chunk(it.first);
    registry.clear();
    chunks.clear();
}

void World::sweep()
{
    for(const auto it : chunks) {
        if(it.second->ref_count >= 0)
            continue;
        remove_chunk(it.first);
    }
}
