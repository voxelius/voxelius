// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_EVENT_VOXEL_SET_HH
#define SHARED_EVENT_VOXEL_SET_HH
#include <shared/chunk.hh>
#include <shared/voxel.hh>

struct VoxelSetEvent final {
    ChunkCoord cv {};
    LocalCoord lv {};
    VoxelCoord vv {};
    Chunk *chunk {};
    Voxel voxel {};
};

#endif /* SHARED_EVENT_VOXEL_SET_HH */
