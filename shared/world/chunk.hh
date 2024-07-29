// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_WORLD_CHUNK_HH
#define SHARED_WORLD_CHUNK_HH
#include <entt/entity/entity.hpp>
#include <shared/world/coord.hh>
#include <shared/world/voxel.hh>

using LightStorage = std::array<short, CHUNK_VOLUME>;
using VoxelStorage = std::array<Voxel, CHUNK_VOLUME>;

struct Chunk final {
    entt::entity entity {};
    VoxelStorage voxels {};
};

struct ChunkComponent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_WORLD_CHUNK_HH */
