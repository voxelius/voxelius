// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>
#include <shared/voxel.hh>

using VoxelStorage = std::array<Voxel, CHUNK_VOLUME>;

struct Chunk final {
    entt::entity entity {};
    VoxelStorage voxels {};
};

struct ChunkComponent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_CHUNK_HH */
