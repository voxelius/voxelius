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

void World::set_voxel(const voxel_pos_t &vpos, voxel_t voxel)
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

void World::set_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos, voxel_t voxel)
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

// https://youtu.be/NbSee-XM7WA
// https://lodev.org/cgtutor/raycasting.html
bool World::raycast(RayDDA &out, const vector3_t &start, const vector3_t &direction, double max_distance) const
{
    Chunk *chunk = nullptr;

    out.start = start;
    out.direction = glm::normalize(direction);

    out.lstep.x = glm::abs(1.0 / out.direction.x);
    out.lstep.y = glm::abs(1.0 / out.direction.y);
    out.lstep.z = glm::abs(1.0 / out.direction.z);

    // FIXME: why exactly can't we use coord::to_voxel() here?
    out.vpos.x = cxmath::floor<voxel_pos_t::value_type>(out.start.x);
    out.vpos.y = cxmath::floor<voxel_pos_t::value_type>(out.start.y);
    out.vpos.z = cxmath::floor<voxel_pos_t::value_type>(out.start.z);

    for(unsigned int d = 0U; d < 3U; ++d) {
        if(out.direction[d] < 0.0) {
            out.lengths[d] = out.lstep[d] * (out.start[d] - out.vpos[d]);
            out.vstep[d] = -1;            
        }
        else {
            out.lengths[d] = out.lstep[d] * (out.vpos[d] + 1.0 - out.start[d]);
            out.vstep[d] = +1;
        }
    }

    out.cpos = coord::to_chunk(out.vpos);
    chunk = find_chunk(out.cpos);
    out.voxel = NULL_VOXEL_ID;

    while(out.distance < max_distance) {
        bool is_done = true;

        for(unsigned int d = 0U; d < 3U; ++d) {
            const double u = out.lengths[(d + 1U) % 3U];
            const double v = out.lengths[(d + 2U) % 3U];

            if(out.lengths[d] < u && out.lengths[d] < v) {
                out.vpos[d] += out.vstep[d];
                out.distance = out.lengths[d];
                out.lengths[d] += out.lstep[d];
                is_done = false;
                break;
            }
        }

        if(is_done) {
            // We didn't propagate any
            // further, we are done then.
            break;
        }

        const auto ncpos = coord::to_chunk(out.vpos);
        if(ncpos != out.cpos) {
            chunk = find_chunk(ncpos);
            out.cpos = ncpos;
        }

        if(chunk) {
            out.voxel = chunk->voxels.at(coord::to_index(coord::to_local(out.vpos)));
            if(out.voxel != NULL_VOXEL) {
                out.wpos = out.start + out.direction * out.distance;
                return true;
            }
        }
    }

    out.wpos = out.start + out.direction * max_distance;
    return false;
}

void World::purge()
{
    for(auto it = chunks.begin(); it != chunks.end();) {
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
