// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk_coord.hh>
#include <mathlib/vec2base.hh>

using ChunkCoord2D = Vec2base<ChunkCoord::value_type>;

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
