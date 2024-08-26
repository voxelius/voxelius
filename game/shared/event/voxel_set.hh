// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk.hh>

struct VoxelSetEvent final {
    std::size_t index {};
    ChunkCoord cpos {};
    LocalCoord lpos {};
    VoxelCoord vpos {};
    Chunk *chunk {};
    Voxel voxel {};    
};