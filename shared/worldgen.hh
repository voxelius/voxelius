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
#ifndef SHARED_WORLDGEN_HH
#define SHARED_WORLDGEN_HH
#include <emhash/hash_table8.hpp>
#include <shared/chunk.hh>
#include <random>

using ChunkNoiseMap = std::array<double, CHUNK_AREA>;

struct World;
struct WorldGen final {
    coord::chunk::value_type max_ccy;
    coord::chunk::value_type min_ccy;
    coord::voxel::value_type sea_level;

    uint64_t world_seed {};
    std::mt19937_64 twister {};
    std::random_device noise {};

    // When generating, multiple chunks have the same
    // horizontal position and only differ vertically.
    // Sampling noise is a somewhat performance heavy
    // operation, so noise map caching is needed.
    emhash8::HashMap<coord::chunk, ChunkNoiseMap> heightmap {};

public:
    static bool generate(World &world, const coord::chunk &cc);
};

#endif /* SHARED_WORLDGEN_HH */
