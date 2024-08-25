// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <entt/entity/entity.hpp>
#include <game/shared/coord.hh>
#include <game/shared/voxel.hh>

using VoxelStorage = std::array<Voxel, CHUNK_VOLUME>;

struct Chunk final {
    entt::entity entity {};
    VoxelStorage voxels {};
};

struct ChunkComponent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};
