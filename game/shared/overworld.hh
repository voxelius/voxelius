// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk.hh>
#include <game/shared/chunk_coord.hh>

namespace overworld
{
void init_late(std::uint64_t seed);
} // namespace overworld

namespace overworld
{
void generate_terrain(const ChunkCoord &cpos, VoxelStorage &voxels);
void generate_surface(const ChunkCoord &cpos, VoxelStorage &voxels);
void generate_carvers(const ChunkCoord &cpos, VoxelStorage &voxels);
void generate_features(const ChunkCoord &cpos, VoxelStorage &voxels);
} // namespace overworld
