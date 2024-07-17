// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <entt/entity/entity.hpp>
#include <shared/voxel_storage.hh>

struct Chunk final {
    entt::entity entity {};
    VoxelStorage storage {};
};

#endif /* SHARED_CHUNK_HH */
