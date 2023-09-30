// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef F8FC1920_3152_4480_B56B_7FF4E9CCD554
#define F8FC1920_3152_4480_B56B_7FF4E9CCD554
#include <array>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <shared/coord.hh>
#include <unordered_map>

using voxel_t = uint32_t;
using voxel_array_t = std::array<voxel_t, CHUNK_VOLUME>;

constexpr static const voxel_t NULL_VOXEL = 0x00000000;
constexpr static const uint32_t NULL_VOXEL_ID = 0x000000;
constexpr static const uint8_t NULL_VOXEL_STATE = 0x00;

constexpr static inline voxel_t make_voxel(uint32_t id, uint8_t state)
{
    voxel_t voxel = 0x00000000;
    voxel |= 0xFF000000 & (state << 24);
    voxel |= 0x00FFFFFF & (id);
    return voxel;
}

constexpr static inline uint32_t get_voxel_id(voxel_t voxel)
{
    return static_cast<uint32_t>((voxel & 0x00FFFFFF));
}

constexpr static inline uint8_t get_voxel_state(voxel_t voxel)
{
    return static_cast<uint8_t>((voxel & 0xFF000000) >> 24);
}

struct Chunk final {
    voxel_array_t voxels {};
    entt::entity entity {entt::null};
};

struct ChunkComponent final {
    Chunk *chunk {nullptr};
    chunk_pos_t cpos {};
};

struct RayDDA final {
    vector3_t start {};
    vector3_t direction {};

    Chunk *chunk {nullptr};
    voxel_t voxel {NULL_VOXEL};
    chunk_pos_t cpos {};
    voxel_pos_t vpos {};
    double distance {};
    vector3_t wpos {};

    vector3_t lstep {};
    vector3_t lengths {};
    voxel_pos_t vstep {};
};

class World final {
public:
    Chunk *create_chunk(const chunk_pos_t &cpos);
    Chunk *find_chunk(const chunk_pos_t &cpos) const;
    void remove_chunk(const chunk_pos_t &cpos);

    voxel_t get_voxel(const voxel_pos_t &vpos) const;
    voxel_t get_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos) const;
    void set_voxel(const voxel_pos_t &vpos, voxel_t voxel);
    void set_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos, voxel_t voxel);

    bool raycast(RayDDA &out, const vector3_t &start, const vector3_t &direction, double max_distance) const;

    void purge();

public:
    entt::registry registry {};

private:
    // FIXME: speed! The standard hashmap implementation is
    // comically slow, use a faster hashmap implementation!
    // The probable candidate is https://github.com/ktprime/emhash
    std::unordered_map<chunk_pos_t, Chunk *> chunks {};
};

struct ChunkCreateEvent final {
    World *world {nullptr};
    Chunk *chunk {nullptr};
    chunk_pos_t cpos {};
};

struct ChunkRemoveEvent final {
    World *world {nullptr};
    Chunk *chunk {nullptr};
    chunk_pos_t cpos {};
};

struct VoxelUpdateEvent final {
    World *world {nullptr};
    Chunk *chunk {nullptr};
    voxel_t voxel {NULL_VOXEL};
    chunk_pos_t cpos {};
    local_pos_t lpos {};
    voxel_pos_t vpos {};
    size_t index {};
};

#endif/* F8FC1920_3152_4480_B56B_7FF4E9CCD554 */
