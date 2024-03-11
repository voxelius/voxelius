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
Chunk *create_chunk(const coord::Chunk &cvec);
Chunk *find_chunk(const coord::Chunk &cvec);
void remove_chunk(const coord::Chunk &cvec);
void purge_chunks(void);
} // namespace world

namespace world
{
uint16_t get_voxel(const coord::Voxel &vvec);
uint16_t get_voxel(const coord::Chunk &cvec, const coord::Local &lvec);
void set_voxel(uint16_t voxel, const coord::Voxel &vvec);
void set_voxel(uint16_t voxel, const coord::Chunk &cvec, const coord::Local &lvec);
} // namespace world

#endif /* SHARED_WORLD_HH */
