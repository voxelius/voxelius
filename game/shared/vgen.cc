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

void vgen::init(std::uint64_t seed)
{
    twister.seed(seed);

    ow_terrain = fnlCreateState();
    ow_terrain.seed = static_cast<int>(twister());
    ow_terrain.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    ow_terrain.fractal_type = FNL_FRACTAL_FBM;
    ow_terrain.frequency = 0.005f;
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

void vgen::generate_overworld(ChunkCoord::value_type cx, ChunkCoord::value_type cz)
{
    // Figure out overworld metrics
    // FIXME: this can be reused; move to init/init_late?
    const std::int64_t ow_height_v = CHUNK_SIZE * static_cast<std::int64_t>(ow_height);
    const std::int32_t ow_half_height = static_cast<std::int32_t>(ow_height >> 1);
    const std::int32_t ow_surface_chunk = static_cast<std::int32_t>(ow_surface >> CHUNK_SIZE_LOG2);
    const std::int32_t ow_start_chunk = ow_surface_chunk - ow_half_height;
    const std::int32_t ow_end_chunk = ow_surface_chunk + ow_half_height;

    // As it turns out LocalCoord::to_index and LocalCoord::from_index
    // both work perfectly fine with values way out of range on the Y axis
    auto voxels = std::vector<Voxel>(CHUNK_AREA * ow_height_v, NULL_VOXEL);

    // FIXME: influence this by a 2D heightmap
    // FIXME: influence surface level by a 2D heightmap as well
    const std::int64_t variation = 64;

    // Generate base terrain
    for(std::size_t idx = 0; idx < voxels.size(); idx += 1) {
        const auto lpos = LocalCoord::from_index(idx);
        const auto vpos = ChunkCoord::to_voxel(ChunkCoord(cx, ow_start_chunk, cz), lpos);
        
        if(cxpr::abs(vpos[1]) > variation) {
            if(vpos[1] < INT64_C(0))
                voxels[idx] = game_voxels::stone;
            else voxels[idx] = NULL_VOXEL;
            continue;
        }
        
        const float density = variation * fnlGetNoise3D(&ow_terrain, vpos[0], vpos[1], vpos[2]) - vpos[1];
        
        if(density > 0.0f)
            voxels[idx] = game_voxels::stone;
        else voxels[idx] = NULL_VOXEL;
    }

    // Sprinkle initial surface voxels
    // This is a pass before we carve caves out
    for(std::int16_t lx = 0; lx < CHUNK_SIZE; lx += 1)
    for(std::int16_t lz = 0; lz < CHUNK_SIZE; lz += 1)
    for(std::int16_t ly = 4; ly < ow_height_v; ly += 1) {
        const auto cdx = LocalCoord::to_index(LocalCoord(lx, ly, lz));
        const auto pdx = LocalCoord::to_index(LocalCoord(lx, ly - 1, lz));
        
        if((voxels[cdx] == NULL_VOXEL) && (voxels[pdx] == game_voxels::stone)) {
            for(std::int16_t bias = 0; bias < 4; bias += 1) {
                const auto bdx = LocalCoord::to_index(LocalCoord(lx, ly - bias - 1, lz));
                if(voxels[bdx] == game_voxels::stone) {
                    if(bias == 0)
                        voxels[bdx] = game_voxels::grass;
                    else voxels[bdx] = game_voxels::dirt;
                }
            }
        }
    }

    // Carve noise caves out
    for(std::size_t idx = 0; idx < voxels.size(); idx += 1) {
        const auto lpos = LocalCoord::from_index(idx);
        const auto vpos = ChunkCoord::to_voxel(ChunkCoord(cx, ow_start_chunk, cz), lpos);

        if(vpos[1] <= variation) {
            const float ca = fnlGetNoise3D(&ow_cave_a, vpos[0], 1.50f * vpos[1], vpos[2]);
            const float cb = fnlGetNoise3D(&ow_cave_b, vpos[0], 1.50f * vpos[1], vpos[2]);
            const float cc = fnlGetNoise3D(&ow_cave_c, vpos[0], 0.75f * vpos[1], vpos[2]);
            
            if((ca * ca + cb * cb + cc * cc) <= 0.015f) {
                voxels[idx] = NULL_VOXEL;
                continue;
            }            
        }
    }

    // Copy voxel data over
    for(std::int32_t cy = 0; cy < ow_height; ++cy) {
        const auto cpos = ChunkCoord(cx, cy + ow_start_chunk, cz);
        const auto offset = CHUNK_VOLUME * cy;

        if(Chunk *chunk = world::assign(cpos, globals::registry.create())) {
            for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
                chunk->voxels[i] = voxels[offset + i];
            }
        }
    }
}
