// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef BA77E979_88AB_4D09_8B83_EF6C14530B74
#define BA77E979_88AB_4D09_8B83_EF6C14530B74
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/shared/chunk.hh>
#include <unordered_map>

class World final {
public:
    Chunk *create_chunk(const chunk_coord_t &cvec);
    Chunk *find_chunk(const chunk_coord_t &cvec) const;
    void remove_chunk(const chunk_coord_t &cvec);

    voxel_t get_voxel(const voxel_coord_t &vvec) const;
    voxel_t get_voxel(const chunk_coord_t &cvec, const local_coord_t &lvec) const;
    void set_voxel(voxel_t voxel, const voxel_coord_t &vvec);
    void set_voxel(voxel_t voxel, const chunk_coord_t &cvec, const local_coord_t &lvec);

    void purge();
    void sweep();

public:
    entt::registry registry {};

private:
    // FIXME: speed! The standard hashmap implementation is
    // comically slow, use a faster hashmap implementation!
    // The probable candidate is https://github.com/ktprime/emhash
    std::unordered_map<chunk_coord_t, Chunk *> chunks {};
};

struct ChunkCreateEvent final {
    World *world {nullptr};
    Chunk *chunk {nullptr};
    chunk_coord_t cvec {};
};

struct ChunkRemoveEvent final {
    World *world {nullptr};
    Chunk *chunk {nullptr};
    chunk_coord_t cvec {};
};

struct VoxelUpdateEvent final {
    World *world {nullptr};
    Chunk *chunk {nullptr};
    voxel_t voxel {NULL_VOXEL};
    chunk_coord_t cvec {};
    local_coord_t lvec {};
    voxel_coord_t vvec {};
    size_t index {};
};

#endif /* BA77E979_88AB_4D09_8B83_EF6C14530B74 */
