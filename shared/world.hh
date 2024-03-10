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
#ifndef SHARED_WORLD_HH
#define SHARED_WORLD_HH
#include <shared/chunk.hh>

namespace world
{
Chunk *create_chunk(const coord::chunk &cvec);
Chunk *find_chunk(const coord::chunk &cvec);
void remove_chunk(const coord::chunk &cvec);
void purge_chunks(void);
} // namespace world

namespace world
{
uint16_t get_voxel(const coord::voxel &vvec);
uint16_t get_voxel(const coord::chunk &cvec, const coord::local &lvec);
void set_voxel(uint16_t voxel, const coord::voxel &vvec);
void set_voxel(uint16_t voxel, const coord::chunk &cvec, const coord::local &lvec);
} // namespace world

#endif /* SHARED_WORLD_HH */
