// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
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
