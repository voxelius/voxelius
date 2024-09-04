// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <functional>
#include <game/shared/const.hh>
#include <mathlib/vec3f.hh>

class ChunkCoord;
class LocalCoord;
class WorldCoord;

class VoxelCoord final : public Vec3base<std::int64_t> {
public:
    template<typename T>
    constexpr VoxelCoord(const Vec3base<T> &other);
    using Vec3base<std::int64_t>::Vec3base;

public:
    static ChunkCoord to_chunk(const VoxelCoord &vvec);
    static LocalCoord to_local(const VoxelCoord &vvec);
    static WorldCoord to_world(const VoxelCoord &vvec);
    static Vec3f to_vec3f(const VoxelCoord &vvec);
};

template<typename T>
constexpr inline VoxelCoord::VoxelCoord(const Vec3base<T> &other)
{
    this[0][0] = static_cast<std::int32_t>(other[0]);
    this[0][1] = static_cast<std::int32_t>(other[1]);
    this[0][2] = static_cast<std::int32_t>(other[2]);
}

