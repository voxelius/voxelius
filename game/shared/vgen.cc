// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <FastNoiseLite.h>
#include <game/shared/game_voxels.hh>
#include <game/shared/globals.hh>
#include <game/shared/vgen.hh>
#include <game/shared/world.hh>
#include <random>

constexpr static std::int32_t OW_HEIGHT = INT32_C(2);
constexpr static std::int32_t OW_OFFSET = INT32_C(-1);
constexpr static std::int64_t OW_V_HEIGHT = CHUNK_SIZE * OW_HEIGHT;
constexpr static std::int64_t OW_V_OFFSET = CHUNK_SIZE * OW_OFFSET;

static fnl_state ow_terrain = {};

void vgen::init(std::uint64_t seed)
{
    std::mt19937_64 twister = std::mt19937_64(seed);

    ow_terrain = fnlCreateState();
    ow_terrain.seed = static_cast<int>(twister());
    ow_terrain.noise_type = FNL_NOISE_OPENSIMPLEX2;
    ow_terrain.fractal_type = FNL_FRACTAL_FBM;
    ow_terrain.frequency = 0.0065f;
    ow_terrain.octaves = 4;
}

void vgen::init_late(void)
{

}

void vgen::gen_islands(ChunkCoord::value_type cx, ChunkCoord::value_type cz)
{
    // not implemented
}

void vgen::gen_overworld(ChunkCoord::value_type cx, ChunkCoord::value_type cz)
{
    std::array<Voxel, CHUNK_VOLUME * OW_HEIGHT> voxels = {};

    for(std::size_t i = 0; i < voxels.size(); ++i) {
        const LocalCoord lpos = LocalCoord::from_index(i);
        const std::int64_t vx = lpos[0] + static_cast<std::int64_t>(cx << CHUNK_SIZE_LOG2);
        const std::int64_t vz = lpos[2] + static_cast<std::int64_t>(cz << CHUNK_SIZE_LOG2);
        const std::int64_t vy = lpos[1] + OW_V_OFFSET;

        const float density = 64.0f * fnlGetNoise3D(&ow_terrain, vx, vy, vz) - vy;

        if(density > 0.0f)
            voxels[i] = game_voxels::stone;
        else voxels[i] = NULL_VOXEL;
    }

    for(std::int16_t x = 0; x < CHUNK_SIZE; ++x) {
        for(std::int16_t z = 0; z < CHUNK_SIZE; ++z) {
            for(std::int16_t y = 4; y < OW_V_HEIGHT; ++y) {
                const auto c_idx = LocalCoord::to_index({x, y, z});

                if(!voxels[c_idx]) {
                    for(std::int16_t b = 0; b < 4; ++b) {
                        const auto p_idx = LocalCoord::to_index(LocalCoord(x, y - b - 1, z));

                        if(voxels[p_idx] == game_voxels::stone) {
                            if(b == 0)
                                voxels[p_idx] = game_voxels::grass;
                            else voxels[p_idx] = game_voxels::dirt;
                        }
                    }
                }
            }
        }
    }

    for(std::int32_t cy = 0; cy < OW_HEIGHT; ++cy) {
        Chunk *chunk = world::assign(ChunkCoord(cx, cy + OW_OFFSET, cz), globals::registry.create());
        std::size_t offset = CHUNK_VOLUME * cy;

        for(std::size_t i = 0; i < CHUNK_VOLUME; ++i) {
            chunk->voxels[i] = voxels[offset + i];
        }
    }
}

void vgen::gen_nether(ChunkCoord::value_type cx, ChunkCoord::value_type cz)
{
    // not implemented
}
