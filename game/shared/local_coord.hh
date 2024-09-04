// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <functional>
#include <game/shared/const.hh>
#include <mathlib/vec3f.hh>

class ChunkCoord;
class VoxelCoord;
class WorldCoord;

class LocalCoord final : public Vec3base<std::int16_t> {
public:
    template<typename T>
    constexpr LocalCoord(const Vec3base<T> &other);
    using Vec3base<std::int16_t>::Vec3base;

public:
    static LocalCoord from_index(const std::size_t index);
    static std::size_t to_index(const LocalCoord &lvec);
    static Vec3f to_vec3f(const LocalCoord &lvec);
};

template<typename T>
constexpr inline LocalCoord::LocalCoord(const Vec3base<T> &other)
{
    this[0][0] = static_cast<std::int16_t>(other[0]);
    this[0][1] = static_cast<std::int16_t>(other[1]);
    this[0][2] = static_cast<std::int16_t>(other[2]);
}
