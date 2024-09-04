// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <functional>
#include <game/shared/const.hh>
#include <mathlib/vec3f.hh>

class LocalCoord;
class VoxelCoord;
class WorldCoord;

class ChunkCoord final : public Vec3base<std::int32_t> {
public:
    template<typename T>
    constexpr ChunkCoord(const Vec3base<T> &other);
    using Vec3base<std::int32_t>::Vec3base;

public:
    static VoxelCoord to_voxel(const ChunkCoord &cvec, std::size_t index);
    static VoxelCoord to_voxel(const ChunkCoord &cvec, const LocalCoord &lvec);
    static WorldCoord to_world(const ChunkCoord &cvec, const Vec3f &lvec);
    static WorldCoord to_world(const ChunkCoord &cvec, const LocalCoord &lvec);
    static WorldCoord to_world(const ChunkCoord &cvec);
    static Vec3f to_vec3f(const ChunkCoord &cvec);
};

template<>
struct std::hash<ChunkCoord> final {
    constexpr inline std::size_t operator()(const ChunkCoord &cvec) const
    {
        std::size_t value = 0;
        value ^= cvec[0] * 73856093;
        value ^= cvec[1] * 19349663;
        value ^= cvec[2] * 83492791;
        return value;
    }
};

template<typename T>
constexpr inline ChunkCoord::ChunkCoord(const Vec3base<T> &other)
{
    this[0][0] = static_cast<std::int64_t>(other[0]);
    this[0][1] = static_cast<std::int64_t>(other[1]);
    this[0][2] = static_cast<std::int64_t>(other[2]);
}
