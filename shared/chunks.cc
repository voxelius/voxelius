// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/chunks.hh>
#include <shared/entity/chunk.hh>
#include <shared/event/chunk_create.hh>
#include <shared/event/chunk_remove.hh>
#include <shared/event/voxel_set.hh>
#include <shared/globals.hh>
#include <unordered_map>

// FIXME: speed! The standard hashmap implementation is
// comically slow, use a faster hashmap implementation!
// The probable candidate is https://github.com/ktprime/emhash
std::unordered_map<chunk_pos_t, Chunk *> map {};

Chunk *chunks::create(const chunk_pos_t &cpos)
{
    Chunk *chunk = chunks::find(cpos);

    if(chunk == nullptr) {
        chunk = new Chunk{};
        chunk->entity = globals::registry.create();
        chunk->voxels.fill(NULL_VOXEL);

        auto &comp = globals::registry.emplace<ChunkComponent>(chunk->entity);
        comp.ptr = chunk;
        comp.cpos = cpos;

        map.emplace(cpos, chunk);

        ChunkCreateEvent event = {};
        event.chunk = chunk;
        event.cpos = cpos;
        globals::dispatcher.trigger(event);
    }

    return chunk;
}

Chunk *chunks::find(const chunk_pos_t &cpos)
{
    if(const auto it = map.find(cpos); it != map.cend())
        return it->second;
    return nullptr;
}

void chunks::remove(const chunk_pos_t &cpos)
{
    if(const auto it = map.find(cpos); it != map.cend()) {
        ChunkCreateEvent event = {};
        event.chunk = it->second;
        event.cpos = cpos;
        globals::dispatcher.trigger(event);

        globals::registry.destroy(it->second->entity);

        delete it->second;

        map.erase(it);
    }
}

void chunks::remove_all()
{
    for(auto it = map.begin(); it != map.end();) {
        ChunkCreateEvent event = {};
        event.chunk = it->second;
        event.cpos = it->first;
        globals::dispatcher.trigger(event);

        globals::registry.destroy(it->second->entity);

        delete it->second;

        it = map.erase(it);
    }

    map.clear();
}

voxel_t chunks::get_voxel(const voxel_pos_t &vpos)
{
    const auto cpos = coord::to_chunk(vpos);
    const auto lpos = coord::to_local(vpos);
    const auto index = coord::to_index(lpos);
    if(const auto it = map.find(cpos); it != map.cend())
        return it->second->voxels[index];
    return NULL_VOXEL;
}

voxel_t chunks::get_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos)
{
    const auto p_vpos = coord::to_voxel(cpos, lpos);
    const auto p_cpos = coord::to_chunk(p_vpos);
    const auto p_lpos = coord::to_local(p_vpos);
    const auto index = coord::to_index(p_lpos);
    if(const auto it = map.find(p_cpos); it != map.cend())
        return it->second->voxels[index];
    return NULL_VOXEL;
}

void chunks::set_voxel(const voxel_pos_t &vpos, voxel_t voxel)
{
    const auto cpos = coord::to_chunk(vpos);
    const auto lpos = coord::to_local(vpos);
    const auto index = coord::to_index(lpos);

    Chunk *chunk = chunks::create(cpos);
    chunk->voxels[index] = voxel;

    VoxelSetEvent event = {};
    event.chunk = chunk;
    event.voxel = voxel;
    event.cpos = cpos;
    event.lpos = lpos;
    event.vpos = vpos;
    event.index = index;
    globals::dispatcher.trigger(event);
}

void chunks::set_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos, voxel_t voxel)
{
    const auto p_vpos = coord::to_voxel(cpos, lpos);
    const auto p_cpos = coord::to_chunk(p_vpos);
    const auto p_lpos = coord::to_local(p_vpos);
    const auto index = coord::to_index(p_lpos);

    Chunk *chunk = chunks::create(p_cpos);
    chunk->voxels[index] = voxel;

    VoxelSetEvent event = {};
    event.chunk = chunk;
    event.voxel = voxel;
    event.cpos = p_cpos;
    event.lpos = p_lpos;
    event.vpos = p_vpos;
    event.index = index;
    globals::dispatcher.trigger(event);
}
