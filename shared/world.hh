// SPDX-License-Identifier: Zlib
#ifndef SHARED_WORLD_HH
#define SHARED_WORLD_HH
#include <shared/chunk.hh>

namespace world
{
Chunk *create_chunk(const ChunkPos &cpos);
Chunk *find_chunk(const ChunkPos &cpos);
void remove_chunk(const ChunkPos &cpos);
void purge_chunks(void);
} // namespace world

namespace world
{
Voxel get_voxel(const VoxelPos &vpos);
Voxel get_voxel(const ChunkPos &cpos, const LocalPos &lpos);
void set_voxel(Voxel voxel, const VoxelPos &vpos);
void set_voxel(Voxel voxel, const ChunkPos &cpos, const LocalPos &lpos);
} // namespace world

#endif /* SHARED_WORLD_HH */
