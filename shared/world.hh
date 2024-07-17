// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_WORLD_HH
#define SHARED_WORLD_HH
#include <shared/chunk.hh>
#include <shared/voxel.hh>

namespace world
{
Chunk *find_or_create_chunk(const ChunkCoord &cv);
Chunk *find_chunk(const ChunkCoord &cv);
void remove_chunk(const ChunkCoord &cv);
void purge_chunks(void);
} // namespace world

namespace world
{
Voxel get_voxel(const VoxelCoord &vv);
Voxel get_voxel(const ChunkCoord &cv, const LocalCoord &lv);
} // namespace world

namespace world
{
void set_voxel(Voxel voxel, const VoxelCoord &vv);
void set_voxel(Voxel voxel, const ChunkCoord &cv, const LocalCoord &lv);
} // namespace world

namespace world
{
void fill_chunks(Voxel voxel, const ChunkCoord &cvx, const ChunkCoord &cvy);
void fill_voxels(Voxel voxel, const VoxelCoord &vvx, const VoxelCoord &vvy);
} // namespace world

// Caching for client-side meshing
namespace world_cache
{
void create(Chunk &target, const Chunk *chunk);
void destroy(Chunk &target);
} // namespace world_cache

#endif /* SHARED_WORLD_HH */
