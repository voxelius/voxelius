// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/vdef.hh>
#include <mathlib/constexpr.hh>
#include <mathlib/vec2f.hh>
#include <mathlib/vec3f.hh>
#include <utility>

// [0] XXXXXXXXXXYYYYYYYYYYZZZZZZZZZZ--
// [1] WWWWWHHHHHFFFFTTTTTTTTTTTAAAAA--
using ChunkQuad = std::array<std::uint32_t, 2>;

constexpr inline static ChunkQuad make_chunk_quad(const Vec3f &position, const Vec2f &size, VoxelFacing facing, std::size_t texture, std::size_t frames)
{
    ChunkQuad result = {};
    result[0] = 0x00000000;
    result[1] = 0x00000000;

    // [0] XXXXXXXXXXYYYYYYYYYYZZZZZZZZZZ--
    result[0] |= (0x000003FFU & static_cast<std::uint32_t>(position[0] * 32.0f)) << 22U;
    result[0] |= (0x000003FFU & static_cast<std::uint32_t>(position[1] * 32.0f)) << 12U;
    result[0] |= (0x000003FFU & static_cast<std::uint32_t>(position[2] * 32.0f)) << 2U;

    // [1] WWWWWHHHHH----------------------
    result[1] |= (0x0000001FU & static_cast<std::uint32_t>(size[0] * 32.0f - 1.0f)) << 27U;
    result[1] |= (0x0000001FU & static_cast<std::uint32_t>(size[1] * 32.0f - 1.0f)) << 22U;

    // [1] ----------FFFF------------------
    result[1] |= (0x0000000FU & static_cast<std::uint32_t>(facing)) << 18U;

    // [1] --------------TTTTTTTTTTTAAAAA--
    result[1] |= (0x000007FFU & static_cast<std::uint32_t>(texture)) << 7U;
    result[1] |= (0x0000001FU & static_cast<std::uint32_t>(frames)) << 2U;

    return std::move(result);
}
