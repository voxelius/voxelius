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

static fnl_state terrain_noise = {};
static fnl_state cave_noise_a = {};
static fnl_state cave_noise_b = {};

void worldgen::init(std::uint64_t seed)
{
    std::mt19937_64 twister = std::mt19937_64(seed);

    terrain_noise = fnlCreateState();
    terrain_noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    terrain_noise.fractal_type = FNL_FRACTAL_RIDGED;
    terrain_noise.seed = static_cast<int>(twister());
    terrain_noise.frequency = 0.0025f;
    terrain_noise.octaves = 4;

    cave_noise_a = fnlCreateState();
    cave_noise_a.noise_type = FNL_NOISE_PERLIN;
    cave_noise_a.seed = static_cast<int>(twister());
    cave_noise_a.frequency = 0.025f;

    cave_noise_b = fnlCreateState();
    cave_noise_b.noise_type = FNL_NOISE_PERLIN;
    cave_noise_b.seed = static_cast<int>(twister());
    cave_noise_b.frequency = 0.025f;
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

    for(std::size_t i = 0; i < CHUNK_AREA; ++i) {
        const auto x = static_cast<std::int16_t>(i % CHUNK_SIZE);
        const auto z = static_cast<std::int16_t>(i / CHUNK_SIZE);
        const auto vpos = ChunkCoord::to_voxel(cpos, LocalCoord(x, 0, z));
        heightmap[i] = SURFACE + VARIATION * fnlGetNoise2D(&terrain_noise, vpos[0], vpos[2]);
    }

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = LocalCoord::from_index(i);
        const auto vpos = ChunkCoord::to_voxel(cpos, lpos);
        const auto surf = heightmap[lpos[0] + lpos[2] * CHUNK_SIZE];

        // What comes next is a big hardcoded bunch of
        // surface layer checks; UNDONE: make it configurable?

        if(vpos[1] <= surf - 32) {
            voxels[i] = game_voxels::slate;
            voxels_empty = false;
            continue;
        }

        if(vpos[1] <= surf - 8) {
            voxels[i] = game_voxels::stone;
            voxels_empty = false;
            continue;
        }

        if(vpos[1] <= surf - 1) {
            voxels[i] = game_voxels::dirt;
            voxels_empty = false;
            continue;
        }

        if(vpos[1] <= surf) {
            voxels[i] = game_voxels::grass;
            voxels_empty = false;
            continue;
        }
    }

    if(voxels_empty) {
        // The chunk is empty, no need to
        // bother with carving or placing features
        return false;
    }

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = LocalCoord::from_index(i);
        const auto vpos = ChunkCoord::to_voxel(cpos, lpos);
        const float ca = fnlGetNoise3D(&cave_noise_a, vpos[0], vpos[1] * 1.5f, vpos[2]);
        const float cb = fnlGetNoise3D(&cave_noise_b, vpos[0], vpos[1] * 1.5f, vpos[2]);

        if((ca * ca + cb * cb) <= 0.0125f) {
            voxels[i] = NULL_VOXEL;
            continue;
        }
    }


    world::assign(cpos, globals::registry.create())->voxels = voxels;

    return true;
}
