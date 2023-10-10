// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CHUNKS_HH
#define SHARED_CHUNKS_HH
#include <array>
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>
#include <shared/voxel.hh>

using voxel_array_t = std::array<voxel_t, CHUNK_VOLUME>;

struct Chunk final {
    entt::entity entity {entt::null};
    voxel_array_t voxels {};
};

namespace chunks
{
Chunk *create(const chunk_pos_t &cpos);
Chunk *find(const chunk_pos_t &cpos);
void remove(const chunk_pos_t &cpos);
void remove_all();

voxel_t get_voxel(const voxel_pos_t &vpos);
voxel_t get_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos);

void set_voxel(const voxel_pos_t &vpos, voxel_t voxel);
void set_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos, voxel_t voxel);
} // namespace chunks

#endif /* SHARED_CHUNKS_HH */
