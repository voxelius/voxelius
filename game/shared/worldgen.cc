// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <algorithm>
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <FastNoiseLite.h>
#include <game/shared/game_voxels.hh>
#include <game/shared/globals.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/voxel_coord.hh>
#include <game/shared/world.hh>
#include <game/shared/worldgen.hh>
#include <mathlib/vec2base.hh>
#include <random>
#include <spdlog/spdlog.h>

enum class ChunkSlice : unsigned int {
    Overworld   = 0x0000,
    Floatlands  = 0x0001,
    Depths      = 0x0002,
};

enum class ProtoStatus : unsigned int {
    Terrain     = 0x0000, // terrain_slicename
    Surface     = 0x0001, // surface_slicename
    Carvers     = 0x0002, // carvers_slicename
    Features    = 0x0003, // features_slicename
    Submit      = 0xFFFF,
};

using ChunkCoord2D = Vec2base<ChunkCoord::value_type>;

struct OverworldMetadata final {
    std::array<std::uint64_t, CHUNK_AREA> entropy {};
    std::array<std::int64_t, CHUNK_AREA> surface {};
};

struct ProtoChunk final {
    ProtoStatus status {};
    ChunkSlice slice {};
    Chunk *chunk {};
};

template<>
struct std::hash<ChunkCoord2D> final {
    constexpr inline std::size_t operator()(const ChunkCoord2D &cvec) const
    {
        std::size_t value = 0;
        value ^= cvec[0] * 73856093;
        value ^= cvec[1] * 19349663;
        return value;
    }
};

static emhash8::HashMap<ChunkCoord2D, OverworldMetadata> overworld_metadata = {};
static emhash8::HashMap<ChunkCoord, ProtoChunk> proto_chunks = {};
static std::mt19937_64 twister = {};

static fnl_state ow_terrain = {};
static fnl_state ow_caves_a = {};
static fnl_state ow_caves_b = {};

// Nominal variation value which should be
// influenced by the biome we're in and other things
constexpr static std::int64_t OW_VARIATION = INT64_C(16);

// Terrain noise is also sampled when we're placing
// surface voxels; this is needed becuase chunks don't
// know if they have generated neighbours or not.
static float get_overworld_noise(const VoxelCoord &vpos, std::int64_t variation)
{
    return variation * fnlGetNoise3D(&ow_terrain, vpos[0], vpos[1], vpos[2]) - vpos[1];
}

static OverworldMetadata &get_overworld_metadata(const ChunkCoord2D &cpos)
{
    const auto it = overworld_metadata.find(cpos);

    if(it == overworld_metadata.cend()) {
        OverworldMetadata &metadata = overworld_metadata.insert_or_assign(cpos, OverworldMetadata()).first->second;
        for(std::size_t i = 0; i < CHUNK_AREA; metadata.entropy[i++] = twister());
        metadata.surface.fill(INT64_MIN);
        return metadata;
    }

    return it->second;
}

static void terrain_overworld(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    OverworldMetadata &metadata = get_overworld_metadata(ChunkCoord2D(cpos[0], cpos[2]));

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::size_t hdx = lpos[0] + lpos[2] * CHUNK_SIZE;

        // Sampling 3D noise like that is expensive; to
        // avoid unnecessary noise sampling we can speculate
        // where the terrain would be guaranteed to be solid or air
        if(cxpr::abs(vpos[1]) >= (OW_VARIATION + 1)) {
            if(vpos[1] < INT64_C(0)) {
                if(vpos[1] > metadata.surface[hdx])
                    metadata.surface[hdx] = vpos[1];
                voxels[index] = game_voxels::stone;
            }
        }

        if(get_overworld_noise(vpos, OW_VARIATION) > 0.0f) {
            if(vpos[1] > metadata.surface[hdx])
                metadata.surface[hdx] = vpos[1];
            voxels[index] = game_voxels::stone;
        }
    }
}

static void surface_overworld(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    OverworldMetadata &metadata = get_overworld_metadata(ChunkCoord2D(cpos[0], cpos[2]));

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
                if(get_overworld_noise(dvpos, OW_VARIATION) <= 0.0f)
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

static void carvers_overworld(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    OverworldMetadata &metadata = get_overworld_metadata(ChunkCoord2D(cpos[0], cpos[2]));

    for(std::size_t index = 0; index < CHUNK_VOLUME; index += 1) {
        const LocalCoord lpos = LocalCoord::from_index(index);
        const VoxelCoord vpos = ChunkCoord::to_voxel(cpos, lpos);
        const std::size_t hdx = lpos[0] + lpos[2] * CHUNK_SIZE;

        // Speculative optimization - there's no solid
        // terrain above OW_VARIATION to carve caves out from
        if(vpos[1] > (OW_VARIATION + 1)) {
            continue;
        }

        const float na = fnlGetNoise3D(&ow_caves_a, vpos[0], 1.5f * vpos[1], vpos[2]);
        const float nb = fnlGetNoise3D(&ow_caves_b, vpos[0], 1.5f * vpos[1], vpos[2]);

        if((na * na + nb * nb) <= (1.0f / 1024.0f)) {
            if(vpos[1] == metadata.surface[hdx])
                metadata.surface[hdx] = INT64_MIN;
            voxels[index] = NULL_VOXEL;
            continue;
        }
    }
}

static void features_overworld(const ChunkCoord &cpos, VoxelStorage &voxels)
{
    OverworldMetadata &metadata = get_overworld_metadata(ChunkCoord2D(cpos[0], cpos[2]));

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
                if(cxpr::range<std::int64_t>(vpos[1] - metadata.surface[hdx], 1, 4))
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

        if(vpos[1] == (metadata.surface[hdx] + 1)) {
            voxels[index] = game_voxels::vtest;
            continue;
        }
    }
#endif
}

void worldgen::init(void)
{
}

void worldgen::init_late(std::uint64_t seed)
{
    twister.seed(seed);

    ow_terrain = fnlCreateState();
    ow_terrain.seed = static_cast<int>(twister());
    ow_terrain.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    ow_terrain.fractal_type = FNL_FRACTAL_FBM;
    ow_terrain.frequency = 0.005f;
    ow_terrain.octaves = 4;

    ow_caves_a = fnlCreateState();
    ow_caves_a.seed = static_cast<int>(twister());
    ow_caves_a.noise_type = FNL_NOISE_PERLIN;
    ow_caves_a.frequency = 0.0075f;

    ow_caves_b = fnlCreateState();
    ow_caves_b.seed = static_cast<int>(twister());
    ow_caves_b.noise_type = FNL_NOISE_PERLIN;
    ow_caves_b.frequency = 0.0075f;
}

void worldgen::deinit(void)
{

}

void worldgen::update(void)
{
    auto it = proto_chunks.begin();
    while(it != proto_chunks.end()) {
        if(it->second.status == ProtoStatus::Terrain) {
            switch(it->second.slice) {
                case ChunkSlice::Overworld: terrain_overworld(it->first, it->second.chunk->voxels); break;
                case ChunkSlice::Floatlands: break;
                case ChunkSlice::Depths: break;
            }

            it->second.status = ProtoStatus::Surface;
            it = std::next(it);
            continue;
        }

        if(it->second.status == ProtoStatus::Surface) {
            switch(it->second.slice) {
                case ChunkSlice::Overworld: surface_overworld(it->first, it->second.chunk->voxels); break;
                case ChunkSlice::Floatlands: break;
                case ChunkSlice::Depths: break;
            }

            it->second.status = ProtoStatus::Carvers;
            it = std::next(it);
            continue;
        }

        if(it->second.status == ProtoStatus::Carvers) {
            switch(it->second.slice) {
                case ChunkSlice::Overworld: carvers_overworld(it->first, it->second.chunk->voxels); break;
                case ChunkSlice::Floatlands: break;
                case ChunkSlice::Depths: break;
            }

            it->second.status = ProtoStatus::Features;
            it = std::next(it);
            continue;
        }

        if(it->second.status == ProtoStatus::Features) {
            switch(it->second.slice) {
                case ChunkSlice::Overworld: features_overworld(it->first, it->second.chunk->voxels); break;
                case ChunkSlice::Floatlands: break;
                case ChunkSlice::Depths: break;
            }

            it->second.status = ProtoStatus::Submit;
            it = std::next(it);
            continue;
        }

        if(it->second.status == ProtoStatus::Submit) {
            spdlog::debug("worldgen: submit {} {} {}", it->first[0], it->first[1], it->first[2]);
            world::emplace_or_replace(it->first, it->second.chunk);
            it = proto_chunks.erase(it);
            continue;
        }

        Chunk::destroy(it->second.chunk);
        it = proto_chunks.erase(it);
    }
}

void worldgen::generate(const ChunkCoord &cpos)
{
    if(proto_chunks.find(cpos) == proto_chunks.cend()) {
        ProtoChunk &pc = proto_chunks.emplace(cpos, ProtoChunk()).first->second;
        pc.chunk = Chunk::create(ChunkType::Generated);
        pc.status = ProtoStatus::Terrain;
        pc.slice = ChunkSlice::Overworld;
    }
}
