// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <array>
#include <entt/entity/entity.hpp>
#include <game/shared/const.hh>
#include <game/shared/voxel.hh>

// A common array type for voxel storage
using VoxelStorage = std::array<Voxel, CHUNK_VOLUME>;

class Chunk final {
public:
    entt::entity entity {};
    VoxelStorage voxels {};

public:
    static Chunk *alloc(void);
    static Chunk *alloc(entt::entity entity);
    static void dealloc(Chunk *chunk);
};
