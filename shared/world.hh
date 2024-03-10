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
#ifndef SHARED_WORLD_HH
#define SHARED_WORLD_HH
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <shared/chunk.hh>
#include <shared/worldgen.hh>

using ChunkHashMap = emhash8::HashMap<coord::chunk, Chunk *>;

struct World final {
    ChunkHashMap chunks {};
    entt::registry registry {};
    WorldGen worldgen {};

public:
    static Chunk *create(World &world, const coord::chunk &cc);
    static Chunk *find(World &world, const coord::chunk &cc);
    static void remove(World &world, const coord::chunk &cc);
    static void purge(World &world);

public:
    static uint16_t get(World &world, const coord::voxel &vv);
    static uint16_t get(World &world, const coord::chunk &cc, const coord::local &ll);
    static void set(World &world, uint16_t voxel, const coord::voxel &vv);
    static void set(World &world, uint16_t voxel, const coord::chunk &cc, const coord::local &ll);
};

#endif /* SHARED_WORLD_HH */
