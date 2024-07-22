// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_TYPES_HH
#define SHARED_TYPES_HH
#include <cstdint>
#include <functional>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

using ChunkPos = glm::vec<3, std::int32_t>;
using LocalPos = glm::vec<3, std::int16_t>;
using VoxelPos = glm::vec<3, std::int64_t>;

using Voxel = std::uint16_t;
constexpr static Voxel VOXEL_MAX = UINT16_MAX;
constexpr static Voxel NULL_VOXEL = 0x0000;

using Light = std::uint8_t;
constexpr static Light LIGHT_SKY_BITS = 0x0F;
constexpr static Light LIGHT_VOX_BITS = 0xF0;
constexpr static Light NULL_LIGHT = 0x00;

template<>
struct std::hash<ChunkPos> final {
    constexpr inline std::size_t operator()(const ChunkPos &cpos) const
    {
        std::size_t value = 0;
        value ^= cpos.x * 73856093;
        value ^= cpos.y * 19349663;
        value ^= cpos.z * 83492791;
        return value;
    }
};

template<>
struct std::hash<VoxelPos> final {
    constexpr inline std::size_t operator()(const VoxelPos &vpos) const
    {
        std::size_t value = 0;
        value ^= vpos.x * 73856093;
        value ^= vpos.y * 19349663;
        value ^= vpos.z * 83492791;
        return value;
    }
};

#endif /* SHARED_TYPES_HH */
