// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk.hh>

namespace world
{
Chunk *create_chunk(const ChunkCoord &cpos, entt::entity entity);
} // namespace world

namespace world
{
Chunk *find_or_create_chunk(const ChunkCoord &cpos);
Chunk *find_chunk(const ChunkCoord &cpos);
void remove_chunk(const ChunkCoord &cpos);
void purge_chunks(void);
} // namespace world

namespace world
{
Voxel get_voxel(const VoxelCoord &vpos);
Voxel get_voxel(const ChunkCoord &cpos, const LocalCoord &lpos);
} // namespace world

namespace world
{
void set_voxel(Voxel voxel, const VoxelCoord &vpos);
void set_voxel(Voxel voxel, const ChunkCoord &cpos, const LocalCoord &lpos);
} // namespace world
