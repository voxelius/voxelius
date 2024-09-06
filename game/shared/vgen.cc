// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <FastNoiseLite.h>
#include <game/shared/game_voxels.hh>
#include <game/shared/globals.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/vgen.hh>
#include <game/shared/voxel_coord.hh>
#include <game/shared/world.hh>
#include <mathlib/vec2base.hh>
#include <random>

constexpr static VoxelCoord::value_type FLOATLANDS_VAR = VoxelCoord::value_type(64);
constexpr static VoxelCoord::value_type FLOATLANDS_MIDDLE = VoxelCoord::value_type(640);
constexpr static ChunkCoord::value_type FLOATLANDS_START_C = ChunkCoord::value_type(2); // prev: 16
constexpr static ChunkCoord::value_type FLOATLANDS_HEIGHT_C = ChunkCoord::value_type(16); // hadcoded
constexpr static VoxelCoord::value_type FLOATLANDS_HEIGHT_V = CHUNK_SIZE * FLOATLANDS_HEIGHT_C;
constexpr static std::size_t FLOATLANDS_VOLUME = CHUNK_VOLUME * FLOATLANDS_HEIGHT_C;

constexpr static VoxelCoord::value_type OVERWORLD_VAR = VoxelCoord::value_type(16); // prev: 64
constexpr static VoxelCoord::value_type OVERWORLD_SURFACE = VoxelCoord::value_type(0);
constexpr static ChunkCoord::value_type OVERWORLD_START_C = ChunkCoord::value_type(-2); // prev: -16
constexpr static ChunkCoord::value_type OVERWORLD_HEIGHT_C = FLOATLANDS_START_C - OVERWORLD_START_C;
constexpr static VoxelCoord::value_type OVERWORLD_HEIGHT_V = CHUNK_SIZE * OVERWORLD_HEIGHT_C;
constexpr static std::size_t OVERWORLD_VOLUME = CHUNK_VOLUME * OVERWORLD_HEIGHT_C;

// Proto-chunks use 2D X/Z chunk coordinates
using ProtoChunkCoord = Vec2base<ChunkCoord::value_type>;

enum class ProtoChunkStage : unsigned int {
    Allocate    = 0x0000,
    Terrain     = 0x0001,
    Surface     = 0x0002,
    Caves       = 0x0003,
    Submit      = 0x0004,
    Zombie      = 0x0005,
};

struct ProtoChunk final {
    std::array<Chunk *, FLOATLANDS_HEIGHT_C> floatlands {};
    std::array<Chunk *, OVERWORLD_HEIGHT_C> overworld {};
    ProtoChunkStage stage {};
};

template<>
struct std::hash<ProtoChunkCoord> final {
    constexpr inline std::size_t operator()(const ProtoChunkCoord &ppos) const
    {
        std::size_t value = 0;
        value ^= ppos[0] * 73856093;
        value ^= ppos[1] * 19349663;
        return value;
    }
};

static emhash8::HashMap<ProtoChunkCoord, ProtoChunk> proto_chunks = {};

static fnl_state ow_terrain = {};
static fnl_state ow_caves_a = {};
static fnl_state ow_caves_b = {};

static std::mt19937_64 twister = {};

static void overworld_allocate(const ProtoChunkCoord &ppos, ProtoChunk &pc)
{
    for(std::size_t i = 0; i < pc.overworld.size(); pc.overworld[i++] = Chunk::alloc());
}

static void overworld_terrain(const ProtoChunkCoord &ppos, ProtoChunk &pc)
{
    for(std::size_t idx = 0; idx < OVERWORLD_VOLUME; idx += 1) {
        const std::size_t chunk_idx = idx / CHUNK_VOLUME;
        const std::size_t local_idx = idx % CHUNK_VOLUME;
        
        const LocalCoord lpos = LocalCoord::from_index(idx);
        const VoxelCoord vpos = ChunkCoord::to_voxel(ChunkCoord(ppos[0], OVERWORLD_START_C + chunk_idx, ppos[1]), lpos);
        const VoxelCoord::value_type surface = vpos[1] + OVERWORLD_SURFACE;
        
        if(cxpr::abs(surface) > OVERWORLD_VAR) {
            if(surface < VoxelCoord::value_type(0))
                pc.overworld[chunk_idx]->voxels[local_idx] = game_voxels::stone;
            continue;
        }

        const float density = OVERWORLD_VAR * fnlGetNoise3D(&ow_terrain, vpos[0], vpos[1], vpos[2]) - surface;

        if(density > 0.0f) {
            pc.overworld[chunk_idx]->voxels[local_idx] = game_voxels::stone;
            continue;
        }
    }
}

static void overworld_surface(const ProtoChunkCoord &ppos, ProtoChunk &pc)
{
    for(LocalCoord::value_type lx = 0; lx < CHUNK_SIZE; lx += 1)
    for(LocalCoord::value_type lz = 0; lz < CHUNK_SIZE; lz += 1)
    for(LocalCoord::value_type ly = 4; ly < OVERWORLD_HEIGHT_V; ly += 1) {
        const VoxelCoord vpos = ChunkCoord::to_voxel(ChunkCoord(ppos[0], OVERWORLD_START_C, ppos[1]), LocalCoord(lx, ly, lz));
        
        if(cxpr::abs(vpos[1]) > OVERWORLD_VAR) {
            // Out of range
            continue;
        }
        
        const std::size_t prev_idx = LocalCoord::to_index(LocalCoord(lx, ly - 1, lz));
        const std::size_t prev_chunk = prev_idx / CHUNK_VOLUME;
        const std::size_t prev_local = prev_idx % CHUNK_VOLUME;
        
        const std::size_t curr_idx = LocalCoord::to_index(LocalCoord(lx, ly, lz));
        const std::size_t curr_chunk = curr_idx / CHUNK_VOLUME;
        const std::size_t curr_local = curr_idx % CHUNK_VOLUME;
        
        const Voxel prev_voxel = pc.overworld[prev_chunk]->voxels[prev_local];
        const Voxel curr_voxel = pc.overworld[curr_chunk]->voxels[curr_local];
        
        if((curr_voxel != NULL_VOXEL) || (prev_voxel != game_voxels::stone)) {
            // Not a surface
            continue;
        }
        
        for(LocalCoord::value_type bias = 0; bias < 4; bias += 1) {
            const std::size_t bias_idx = LocalCoord::to_index(LocalCoord(lx, ly - bias - 1, lz));
            const std::size_t bias_chunk = bias_idx / CHUNK_VOLUME;
            const std::size_t bias_local = bias_idx % CHUNK_VOLUME;
            
            if(pc.overworld[bias_chunk]->voxels[bias_local] == game_voxels::stone) {
                if(bias == 0)
                    pc.overworld[bias_chunk]->voxels[bias_local] = game_voxels::grass;
                else pc.overworld[bias_chunk]->voxels[bias_local] = game_voxels::dirt;
            }
        }
    }
}

static void overworld_caves(const ProtoChunkCoord &ppos, ProtoChunk &pc)
{
    for(std::size_t idx = 0; idx < OVERWORLD_VOLUME; idx += 1) {
        const std::size_t chunk_idx = idx / CHUNK_VOLUME;
        const std::size_t local_idx = idx % CHUNK_VOLUME;
        
        const LocalCoord lpos = LocalCoord::from_index(idx);
        const VoxelCoord vpos = ChunkCoord::to_voxel(ChunkCoord(ppos[0], OVERWORLD_START_C + chunk_idx, ppos[1]), lpos);
        const VoxelCoord::value_type surface = vpos[1] + OVERWORLD_SURFACE;
        
        if(surface > OVERWORLD_VAR) {
            // Can't carve air
            continue;
        }

        const float na = fnlGetNoise3D(&ow_caves_a, vpos[0], 1.5f * vpos[1], vpos[2]);
        const float nb = fnlGetNoise3D(&ow_caves_b, vpos[0], 1.5f * vpos[1], vpos[2]);

        if((na * na + nb * nb) <= (1.0f / 1024.0f)) {
            pc.overworld[chunk_idx]->voxels[local_idx] = NULL_VOXEL;
            continue;
        }
    }
}

static void overworld_submit(const ProtoChunkCoord &ppos, ProtoChunk &pc)
{
    for(ChunkCoord::value_type cy = 0; cy < pc.overworld.size(); cy += 1) {
        pc.overworld[cy]->entity = globals::registry.create();
        world::assign(ChunkCoord(ppos[0], OVERWORLD_START_C + cy, ppos[1]), pc.overworld[cy]);
        pc.overworld[cy] = nullptr;
    }
}

void vgen::init(std::uint64_t seed)
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

void vgen::init_late(void)
{
}

void vgen::deinit(void)
{
}

void vgen::update(void)
{
    for(auto &it : proto_chunks) {
        if(it.second.stage == ProtoChunkStage::Allocate) {
            overworld_allocate(it.first, it.second);
            it.second.stage = ProtoChunkStage::Terrain;
            continue;
        }
        
        if(it.second.stage == ProtoChunkStage::Terrain) {
            overworld_terrain(it.first, it.second);
            it.second.stage = ProtoChunkStage::Surface;
            continue;
        }
        
        if(it.second.stage == ProtoChunkStage::Surface) {
            overworld_surface(it.first, it.second);
            it.second.stage = ProtoChunkStage::Caves;
            continue;
        }
        
        if(it.second.stage == ProtoChunkStage::Caves) {
            overworld_caves(it.first, it.second);
            it.second.stage = ProtoChunkStage::Submit;
            continue;
        }
        
        if(it.second.stage == ProtoChunkStage::Submit) {
            overworld_submit(it.first, it.second);
            it.second.stage = ProtoChunkStage::Zombie;
            continue;
        }
    }
}

bool vgen::generate(ChunkCoord::value_type cx, ChunkCoord::value_type cz)
{
    const ProtoChunkCoord ppos = ProtoChunkCoord(cx, cz);
    const auto it = proto_chunks.find(ppos);
    
    if(it == proto_chunks.cend()) {
        ProtoChunk &pc = proto_chunks.emplace(ppos, ProtoChunk()).first->second;
        pc.stage = ProtoChunkStage::Allocate;
        return true;
    }
    
    return false;
}
