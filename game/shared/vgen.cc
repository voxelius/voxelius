// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <FastNoiseLite.h>
#include <game/shared/game_voxels.hh>
#include <game/shared/globals.hh>
#include <game/shared/vgen.hh>
#include <game/shared/world.hh>
#include <random>

static std::mt19937_64 twister = {};

static fnl_state ow_terrain = {};
static fnl_state ow_cave_a = {};
static fnl_state ow_cave_b = {};
static fnl_state ow_cave_c = {};

static std::int32_t ow_height = 8;
static std::int64_t ow_surface = 0;

static bool ow_generate(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    bool empty_chunk = true;
    
    // This defines variation scale (amplification);
    // UNDONE: influence this via a second 2D noise map
    const std::int64_t ow_variation = static_cast<std::int64_t>(32);

    // This contains sampled surface levels
    // FIXME: the heightmap can be reused for all the chunks
    // with the same X and Z coordinates; store it somewhere?
    std::array<std::int64_t, CHUNK_AREA> heightmap = {};

    for(std::size_t i = 0; i < CHUNK_AREA; ++i) {
        const std::int16_t lx = static_cast<std::int16_t>(i % CHUNK_SIZE);
        const std::int16_t lz = static_cast<std::int16_t>(i / CHUNK_SIZE);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, LocalCoord(lx, 0, lz));
        heightmap[i] = ow_surface + ow_variation * fnlGetNoise2D(&ow_terrain, vpos[0], vpos[2]);
    }

    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = LocalCoord::from_index(i);
        const auto vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::int64_t surf = heightmap[lpos[0] + lpos[2] * CHUNK_SIZE];

        if(vpos[1] <= ow_surface - 96 + (static_cast<std::int64_t>(twister()) % 4)) {
            voxels[i] = game_voxels::slate;
            empty_chunk = false;
            continue;
        }

        if(vpos[1] <= surf - 8) {
            voxels[i] = game_voxels::stone;
            empty_chunk = false;
            continue;
        }

        if(vpos[1] <= surf - 1) {
            voxels[i] = game_voxels::dirt;
            empty_chunk = false;
            continue;
        }

        if(vpos[1] <= surf) {
            voxels[i] = game_voxels::grass;
            empty_chunk = false;
            continue;
        }
    }

    if(empty_chunk) {
        // There is no point in carving
        // out cave systems from an empty chunk
        return false;
    }

/*
    for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
        const auto lpos = LocalCoord::from_index(i);
        const auto vpos = ChunkCoord::to_voxel(cpos, lpos);
        const float ca = fnlGetNoise3D(&ow_cave_a, vpos[0], 1.50f * vpos[1], vpos[2]);
        const float cb = fnlGetNoise3D(&ow_cave_b, vpos[0], 1.50f * vpos[1], vpos[2]);
        const float cc = fnlGetNoise3D(&ow_cave_c, vpos[0], 0.75f * vpos[1], vpos[2]);
        
        if((ca * ca + cb * cb + 0.5f * cc * cc * cc * cc) <= 0.005f) {
            voxels[i] = NULL_VOXEL;
            continue;
        }
    }
*/

    return true;
}

void vgen::init(std::uint64_t seed)
{
    twister.seed(seed);

    ow_terrain = fnlCreateState();
    ow_terrain.seed = static_cast<int>(twister());
    ow_terrain.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    ow_terrain.fractal_type = FNL_FRACTAL_RIDGED;
    ow_terrain.frequency = 0.0025f;
    ow_terrain.octaves = 4;

    ow_cave_a = fnlCreateState();
    ow_cave_a.seed = static_cast<int>(twister());
    ow_cave_a.noise_type = FNL_NOISE_PERLIN;
    ow_cave_a.frequency = 0.025f;

    ow_cave_b = fnlCreateState();
    ow_cave_b.seed = static_cast<int>(twister());
    ow_cave_b.noise_type = FNL_NOISE_PERLIN;
    ow_cave_b.frequency = 0.025f;

    ow_cave_c = fnlCreateState();
    ow_cave_c.seed = static_cast<int>(twister());
    ow_cave_c.noise_type = FNL_NOISE_PERLIN;
    ow_cave_c.frequency = 0.0125f;
}

void vgen::init_late(void)
{

}

void vgen::deinit(void)
{
    
}

void vgen::generate(const ChunkCoord &cpos)
{
    // Figure out overworld metrics
    // FIXME: this can be reused; move to init/init_late?
    const std::int32_t ow_half_height = static_cast<std::int32_t>(ow_height >> 1);
    const std::int32_t ow_surface_chunk = static_cast<std::int32_t>(ow_surface >> CHUNK_SIZE_LOG2);
    const std::int32_t ow_start_chunk = ow_surface_chunk - ow_half_height;
    const std::int32_t ow_end_chunk = ow_surface_chunk + ow_half_height;
    
    VoxelStorage voxels = {};
    voxels.fill(NULL_VOXEL);
    
    if((cpos[1] >= ow_start_chunk) && (cpos[1] <= ow_end_chunk)) {
        if(ow_generate(cpos, voxels))
            world::assign(cpos, globals::registry.create())->voxels = voxels;
        return;
    }
}
