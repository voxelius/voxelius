// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <FastNoiseLite.h>
#include <game/shared/game_voxels.hh>
#include <game/shared/globals.hh>
#include <game/shared/world.hh>
#include <game/shared/worldgen.hh>
#include <random>

constexpr static std::int64_t SURFACE = 0;
constexpr static std::int64_t VARIATION = 16;

static fnl_state cnoise = {};
static fnl_state tnoise = {};

void worldgen::init(std::uint64_t seed)
{
    cnoise = fnlCreateState();
    cnoise.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    cnoise.fractal_type = FNL_FRACTAL_NONE;
    cnoise.seed = static_cast<int>(std::mt19937_64(seed)());
    cnoise.frequency = 0.025f;

    tnoise = fnlCreateState();
    tnoise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    tnoise.fractal_type = FNL_FRACTAL_FBM;
    tnoise.seed = static_cast<int>(seed);
    tnoise.frequency = 0.0025f;
    tnoise.octaves = 4;
}

void worldgen::deinit(void)
{

}

void worldgen::update(void)
{

}

bool worldgen::generate(const ChunkCoord &cpos)
{
    std::array<std::int64_t, CHUNK_AREA> heightmap = {};
    std::array<Voxel, CHUNK_VOLUME> voxels = {};
    bool voxels_empty = true;

    heightmap.fill(INT64_MIN);
    voxels.fill(NULL_VOXEL);

    for(std::size_t index = 0; index < CHUNK_AREA; index += 1U) {
        const std::int16_t x = static_cast<std::int16_t>(index % CHUNK_SIZE);
        const std::int16_t z = static_cast<std::int16_t>(index / CHUNK_SIZE);
        const auto vpos = ChunkCoord::to_voxel(cpos, LocalCoord(x, 0, z));
        heightmap[index] = SURFACE + VARIATION * fnlGetNoise2D(&tnoise, vpos[0], vpos[2]);
    }

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1U) {
        const auto lpos = LocalCoord::from_index(index);
        const auto vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::int64_t surf = heightmap[CHUNK_SIZE * lpos[2] + lpos[0]];

        // What follows is a bunch of hard-coded
        // height values that make the terrain to look like it does
        // UNDONE: implement a layered system for this

        if(vpos[1] <= surf - 32) {
            voxels[index] = game_voxels::slate;
            voxels_empty = false;
            continue;
        }

        if(vpos[1] <= surf - 8) {
            voxels[index] = game_voxels::stone;
            voxels_empty = false;
            continue;
        }

        if(vpos[1] <= surf - 1) {
            voxels[index] = game_voxels::dirt;
            voxels_empty = false;
            continue;
        }

        if(vpos[1] <= surf) {
            voxels[index] = game_voxels::grass;
            voxels_empty = false;
            continue;
        }
    }

    if(voxels_empty) {
        // The chunk is empty, no need to
        // bother with carving or placing features
        return false;
    }

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1U) {
        const auto lpos = LocalCoord::from_index(index);
        const auto vpos = ChunkCoord::to_voxel(cpos, lpos);
        float carve = fnlGetNoise3D(&cnoise, vpos[0], vpos[1], vpos[2]);

        if(carve >= 0.50f) {
            voxels[index] = NULL_VOXEL;
            continue;
        }
    }

    world::assign(cpos, globals::registry.create())->voxels = voxels;

    return true;
}
