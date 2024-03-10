// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <array>
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>

constexpr static uint16_t NULL_VOXEL = 0x0000;
constexpr static uint8_t NULL_LIGHT = 0x00;

struct Chunk final {
    entt::entity entity {};
    std::array<uint16_t, CHUNK_VOLUME> voxels {};
    std::array<uint8_t, CHUNK_VOLUME> lightmap {};
};

struct ChunkComponent final {
    coord::chunk position {};
    Chunk *chunk {nullptr};
};

#endif /* SHARED_CHUNK_HH */
