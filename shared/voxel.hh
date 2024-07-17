// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_VOXEL_HH
#define SHARED_VOXEL_HH
#include <cstdint>

using Voxel = std::uint32_t;
constexpr static Voxel VOXEL_MAX = UINT32_MAX;
constexpr static Voxel NULL_VOXEL = 0x00000000;

// Must fit within voxels field of Chunk for
// ChunkStorageType::FILL_VOXEL storage to work
static_assert(sizeof(Voxel) <= sizeof(void *));

#endif /* SHARED_VOXEL_HH */
