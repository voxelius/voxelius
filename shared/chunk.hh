// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <array>
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>
#include <shared/voxel.hh>

using VoxelStorage = std::array<Voxel, CHUNK_VOLUME>;

struct Chunk final {
    entt::entity entity {};
    VoxelStorage voxels {};
};

#endif /* SHARED_CHUNK_HH */
