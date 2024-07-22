// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <array>
#include <entt/entity/entity.hpp>
#include <shared/const.hh>
#include <shared/types.hh>

using LightArray = std::array<Light, CHUNK_VOLUME>;
using VoxelArray = std::array<Voxel, CHUNK_VOLUME>;

struct Chunk final {
    entt::entity entity {};
    LightArray lightmap {};
    VoxelArray voxels {};
};

#endif /* SHARED_CHUNK_HH */
