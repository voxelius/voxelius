// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk.hh>

namespace world
{
void init(void);
} // namespace world

namespace world
{
void emplace_or_replace(const ChunkCoord &cpos, Chunk *object);
Chunk *find(const ChunkCoord &cpos);
Chunk *find(entt::entity entity);
} // namespace world

namespace world
{
Voxel get_voxel(const VoxelCoord &vpos);
Voxel get_voxel(const ChunkCoord &cpos, const LocalCoord &lpos);
bool set_voxel(Voxel voxel, const VoxelCoord &vpos);
bool set_voxel(Voxel voxel, const ChunkCoord &cpos, const LocalCoord &lpos);
} // namespace world
