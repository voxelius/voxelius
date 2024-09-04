// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk.hh>

constexpr static std::int64_t FLOATLANDS_VAR = INT64_C(64);
constexpr static std::int64_t FLOATLANDS_MIDDLE = INT64_C(640);
constexpr static std::int32_t FLOATLANDS_START_C = INT32_C(16);
constexpr static std::int32_t FLOATLANDS_HEIGHT_C = INT32_C(16); // hardcoded

constexpr static std::int64_t OVERWORLD_VAR = INT64_C(64);
constexpr static std::int64_t OVERWORLD_SURFACE = INT64_C(0);
constexpr static std::int32_t OVERWORLD_START_C = INT32_C(-16);
constexpr static std::int32_t OVERWORLD_HEIGHT_C = FLOATLANDS_START_C - OVERWORLD_START_C;

constexpr static std::int64_t DEPTHS_VAR1 = INT64_C(64);
constexpr static std::int64_t DEPTHS_VAR2 = INT64_C(16);
constexpr static std::int64_t DEPTHS_MIDDLE = INT64_C(-640);
constexpr static std::int32_t DEPTHS_START_C = INT32_C(-24);
constexpr static std::int32_t DEPTHS_HEIGHT_C = OVERWORLD_START_C - DEPTHS_START_C;

enum class ProtoStatus : unsigned int {
    Empty   = 0x0000, // The protochunk has just been initialized
    Biomes  = 0x0001, // Biome map is sampled and stored in the chunks
    Terrain = 0x0002, // Slice-specific terrain is generated
    Surface = 0x0003, // Slice and biome-specific surface voxels are placed
    Carvers = 0x0004, // Caves are carved out (possibly skipped for floatlands)
    Feature = 0x0005, // Features and structures are placed
    Submit  = 0x0006, // Allocated chunks ownership is transferred to [world]
};

struct ProtoChunk final {
    std::array<Chunk *, FLOATLANDS_HEIGHT_C> floatlands_chunks {};
    std::array<Chunk *, OVERWORLD_HEIGHT_C> overworld_chunks {};
    std::array<Chunk *, DEPTHS_HEIGHT_C> depths_chunks {};
    ProtoStatus floatlands_status {};
    ProtoStatus overworld_status {};
    ProtoStatus depths_status {};
};

namespace vgen
{
void init(std::uint64_t seed);
void init_late(void);
void deinit(void);
void update_late(void);
} // namespace vgen

namespace vgen
{
void enqueue(ChunkCoord::value_type cx, ChunkCoord::value_type cz);
} // namespace vgen
