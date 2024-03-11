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

// UNDONE: I am going to bed now
// TURN THIS INTO A NAMESPACE (PICKLE) TOMORROW

struct NoiseCache final {
    int16_t biome_temperature {25};
    uint16_t biome_humidity {15};
    double heightmap[CHUNK_VOLUME] {};
};

struct WorldGen final {
    uint64_t seed {5489U};
    double horizontal_scale {1.0};
    double vertical_scale {1.0};

    coord::Chunk::value_type bottom {0};
    coord::Chunk::value_type height {16};
    coord::Voxel::value_type sea_level {64};

    coord::Chunk::value_type g_top {};
    coord::Voxel::value_type g_height {};

    std::mt19937_64 twister {};
    std::random_device noise {};

    emhash8::HashMap<coord::Chunk, NoiseCache> cache {};

public:
    static void init(WorldGen &worldgen);
    static bool generate(WorldGen &WorldGen, Chunk *chunk, const coord::Chunk &cvec);
};

#endif /* SHARED_WORLDGEN_HH */
