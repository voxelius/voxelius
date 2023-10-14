// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CHUNKS_HH
#define SHARED_CHUNKS_HH
#include <shared/chunk.hh>
#include <shared/coord.hh>

namespace world
{
Chunk *create_chunk(const chunk_pos_t &cpos);
Chunk *find_chunk(const chunk_pos_t &cpos);
void remove_chunk(const chunk_pos_t &cpos);
void remove_all_chunks();

voxel_t get_voxel(const voxel_pos_t &vpos);
voxel_t get_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos);

void set_voxel(const voxel_pos_t &vpos, voxel_t voxel);
void set_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos, voxel_t voxel);
} // namespace world

#endif /* SHARED_CHUNKS_HH */
