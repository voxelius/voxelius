// SPDX-License-Identifier: Zlib
#ifndef EVENT_VOXEL_SET_HH
#define EVENT_VOXEL_SET_HH
#include <shared/chunk.hh>

struct VoxelSetEvent final {
    ChunkCoord cv {};
    LocalCoord lv {};
    VoxelCoord vv {};
    Chunk *chunk {};
    Voxel voxel {};
};

#endif /* EVENT_VOXEL_SET_HH */
