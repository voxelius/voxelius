// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <algorithm>
#include <emhash/hash_table8.hpp>
#include <entt/entity/registry.hpp>
#include <FastNoiseLite.h>
#include <game/shared/game_voxels.hh>
#include <game/shared/globals.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/overworld.hh>
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

struct ProtoChunk final {
    ProtoStatus status {};
    ChunkSlice slice {};
    Chunk *chunk {};
};

static emhash8::HashMap<ChunkCoord, ProtoChunk> proto_chunks = {};

void worldgen::init(void)
{
}

void worldgen::init_late(std::uint64_t seed)
{
    overworld::init_late(seed);
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
                case ChunkSlice::Overworld:
                    overworld::generate_terrain(it->first, it->second.chunk->voxels);
                    break;
                case ChunkSlice::Floatlands: break;
                case ChunkSlice::Depths: break;
            }

            it->second.status = ProtoStatus::Surface;
            it = std::next(it);
            continue;
        }

        if(it->second.status == ProtoStatus::Surface) {
            switch(it->second.slice) {
                case ChunkSlice::Overworld:
                    overworld::generate_surface(it->first, it->second.chunk->voxels);
                    break;
                case ChunkSlice::Floatlands: break;
                case ChunkSlice::Depths: break;
            }

            it->second.status = ProtoStatus::Carvers;
            it = std::next(it);
            continue;
        }

        if(it->second.status == ProtoStatus::Carvers) {
            switch(it->second.slice) {
                case ChunkSlice::Overworld:
                    overworld::generate_carvers(it->first, it->second.chunk->voxels);
                    break;
                case ChunkSlice::Floatlands: break;
                case ChunkSlice::Depths: break;
            }

            it->second.status = ProtoStatus::Features;
            it = std::next(it);
            continue;
        }

        if(it->second.status == ProtoStatus::Features) {
            switch(it->second.slice) {
                case ChunkSlice::Overworld:
                    overworld::generate_features(it->first, it->second.chunk->voxels);
                    break;
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
