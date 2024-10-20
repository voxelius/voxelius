// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <emhash/hash_table8.hpp>
#include <FastNoiseLite.h>
#include <game/shared/chunk_coord_2d.hh>
#include <game/shared/game_voxels.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/overworld.hh>
#include <game/shared/voxel_coord.hh>
#include <random>

struct Metadata final {
    std::array<std::uint64_t, CHUNK_AREA> entropy {};
    std::array<std::int64_t, CHUNK_AREA> heightmap {};
};

static emhash8::HashMap<ChunkCoord2D, Metadata> metadata_map = {};
static std::mt19937_64 twister = {};
static fnl_state fnl_terrain = {};
static fnl_state fnl_caves_a = {};
static fnl_state fnl_caves_b = {};

// Nominal variation value which should be
// influenced by the biome we're in and other things
constexpr static std::int64_t OW_VARIATION = INT64_C(64);

// Terrain noise is also sampled when we're placing
// surface voxels; this is needed becuase chunks don't
// know if they have generated neighbours or not.
static float get_noise(const VoxelCoord &vpos, std::int64_t variation)
{
    return variation * fnlGetNoise3D(&fnl_terrain, vpos[0], vpos[1], vpos[2]) - vpos[1];
}

static Metadata &get_metadata(const ChunkCoord2D &cpos)
{
    const auto it = metadata_map.find(cpos);

    if(it == metadata_map.cend()) {
        Metadata &metadata = metadata_map.insert_or_assign(cpos, Metadata()).first->second;
        for(std::size_t i = 0; i < CHUNK_AREA; ++i)
            metadata.entropy[i] = twister();
        metadata.heightmap.fill(INT64_MIN);
        return metadata;
    }

    return it->second;
}

void overworld::init_late(std::uint64_t seed)
{
    twister.seed(seed);

    fnl_terrain = fnlCreateState();
    fnl_terrain.seed = static_cast<int>(twister());
    fnl_terrain.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    fnl_terrain.fractal_type = FNL_FRACTAL_FBM;
    fnl_terrain.frequency = 0.005f;
    fnl_terrain.octaves = 4;

    fnl_caves_a = fnlCreateState();
    fnl_caves_a.seed = static_cast<int>(twister());
    fnl_caves_a.noise_type = FNL_NOISE_PERLIN;
    fnl_caves_a.frequency = 0.0075f;

    fnl_caves_b = fnlCreateState();
    fnl_caves_b.seed = static_cast<int>(twister());
    fnl_caves_b.noise_type = FNL_NOISE_PERLIN;
    fnl_caves_b.frequency = 0.0075f;
}

void overworld::generate_terrain(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    Metadata &metadata = get_metadata(ChunkCoord2D(cpos[0], cpos[2]));

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::size_t hdx = lpos[0] + lpos[2] * CHUNK_SIZE;

        // Sampling 3D noise like that is expensive; to
        // avoid unnecessary noise sampling we can speculate
        // where the terrain would be guaranteed to be solid or air
        if(cxpr::abs(vpos[1]) >= (OW_VARIATION + 1)) {
            if(vpos[1] < INT64_C(0)) {
                if(vpos[1] > metadata.heightmap[hdx])
                    metadata.heightmap[hdx] = vpos[1];
                voxels[index] = game_voxels::stone;
            }
        }

        if(get_noise(vpos, OW_VARIATION) > 0.0f) {
            if(vpos[1] > metadata.heightmap[hdx])
                metadata.heightmap[hdx] = vpos[1];
            voxels[index] = game_voxels::stone;
        }
    }
}

void overworld::generate_surface(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    Metadata &metadata = get_metadata(ChunkCoord2D(cpos[0], cpos[2]));

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);

        // Same speculation check applies here albeit
        // a little differently - there's no surface to
        // place voxels on above OW_VARIATION range
        if(cxpr::abs(vpos[1]) >= (OW_VARIATION + 1)) {
            continue;
        }

        // Surface voxel checks only apply for solid voxels;
        // it's kind of obvious you can't replace air with grass
        if(voxels[index] == NULL_VOXEL) {
            continue;
        }

        std::size_t depth = 0;

        for(std::int16_t dy = 0; dy < 5; dy += 1) {
            const LocalCoord dlpos = LocalCoord(lpos[0], lpos[1] + dy + 1, lpos[2]);
            const VoxelCoord dvpos = ChunkCoord::to_voxel(cpos, dlpos);
            const std::size_t didx = LocalCoord::to_index(dlpos);

            if(dlpos[1] >= CHUNK_SIZE) {
                if(get_noise(dvpos, OW_VARIATION) <= 0.0f)
                    break;
                depth += 1;
            }
            else {
                if(voxels[didx] == NULL_VOXEL)
                    break;
                depth += 1;
            }
        }

        if(depth < 5) {
            if(depth == 0)
                voxels[index] = game_voxels::grass;
            else voxels[index] = game_voxels::dirt;
        }
    }
}

void overworld::generate_carvers(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    Metadata &metadata = get_metadata(ChunkCoord2D(cpos[0], cpos[2]));

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::size_t hdx = lpos[0] + lpos[2] * CHUNK_SIZE;

        // Speculative optimization - there's no solid
        // terrain above OW_VARIATION to carve caves out from
        if(vpos[1] > (OW_VARIATION + 1)) {
            continue;
        }

        const float na = fnlGetNoise3D(&fnl_caves_a, vpos[0], 1.5f * vpos[1], vpos[2]);
        const float nb = fnlGetNoise3D(&fnl_caves_b, vpos[0], 1.5f * vpos[1], vpos[2]);

        if((na * na + nb * nb) <= (1.0f / 1024.0f)) {
            if(vpos[1] == metadata.heightmap[hdx])
                metadata.heightmap[hdx] = INT64_MIN;
            voxels[index] = NULL_VOXEL;
            continue;
        }
    }
}

void overworld::generate_features(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    Metadata &metadata = get_metadata(ChunkCoord2D(cpos[0], cpos[2]));

    // Replace all stone with slate below -64
    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);

        if(voxels[index] == game_voxels::stone) {
            if(vpos[1] <= ((static_cast<std::int64_t>(twister()) % 8) - 64)) {
                voxels[index] = game_voxels::slate;
                continue;
            }
        }
    }

#if 1
    constexpr static std::size_t COUNT = 5;
    std::array<std::int16_t, COUNT> lxa = {};
    std::array<std::int16_t, COUNT> lza = {};
    std::array<std::int64_t, COUNT> heights = {};

    for(std::size_t tc = 0; tc < COUNT; tc += 1) {
        lxa[tc] = static_cast<std::int16_t>(metadata.entropy[tc * 3 + 0] % CHUNK_SIZE);
        lza[tc] = static_cast<std::int16_t>(metadata.entropy[tc * 3 + 1] % CHUNK_SIZE);
        heights[tc] = 3 + static_cast<std::int64_t>(metadata.entropy[tc * 3 + 2] % 4);
    }

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::size_t hdx = lpos[0] + lpos[2] * CHUNK_SIZE;

        for(std::size_t tc = 0; tc < COUNT; tc += 1) {
            if((lpos[0] == lxa[tc]) && (lpos[2] == lza[tc])) {
                if(cxpr::range<std::int64_t>(vpos[1] - metadata.heightmap[hdx], 1, 4))
                    voxels[index] = game_voxels::vtest;
                break;
            }
        }
    }
#else
    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::size_t hdx = lpos[0] + lpos[2] * CHUNK_SIZE;

        if(vpos[1] == (metadata.heightmap[hdx] + 1)) {
            voxels[index] = game_voxels::vtest;
            continue;
        }
    }
#endif
}
