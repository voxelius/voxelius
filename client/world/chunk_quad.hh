// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_WORLD_CHUNK_QUAD_HH
#define CLIENT_WORLD_CHUNK_QUAD_HH
#include <shared/math/constexpr.hh>
#include <shared/math/vec2f.hh>
#include <shared/math/vec3f.hh>
#include <shared/world/vdef.hh>
#include <utility>

// [0] XXXXXXXXYYYYYYYYZZZZZZZZWWWWHHHH
// [1] FFFFTTTTTTTTTTTAAAAA------------
using ChunkQuad = std::array<std::uint32_t, 2>;

constexpr inline static ChunkQuad make_chunk_quad(const Vec3f &position, const Vec2f &size, VoxelFacing facing, std::size_t texture, std::size_t frames)
{
    ChunkQuad result = {};
    result[0] = 0x00000000;
    result[1] = 0x00000000;

    // [0] XXXXXXXXYYYYYYYYZZZZZZZZ--------
    result[0] |= (0x000000FF & static_cast<std::uint32_t>(position[0] * 16.0f)) << 24;
    result[0] |= (0x000000FF & static_cast<std::uint32_t>(position[1] * 16.0f)) << 16;
    result[0] |= (0x000000FF & static_cast<std::uint32_t>(position[2] * 16.0f)) << 8;

    // [0] ------------------------WWWWHHHH
    result[0] |= (0x0000000F & static_cast<std::uint32_t>(size[0] * 16.0f - 1.0f)) << 4;
    result[0] |= (0x0000000F & static_cast<std::uint32_t>(size[1] * 16.0f - 1.0f));

    // [1] FFFF----------------------------
    result[1] |= (0x0000000F & static_cast<std::uint32_t>(facing)) << 28;

    // [1] ----TTTTTTTTTTTAAAAA------------
    result[1] |= (0x000007FF & static_cast<std::uint32_t>(texture)) << 17;
    result[1] |= (0x0000001F & static_cast<std::uint32_t>(frames)) << 12;

    return std::move(result);
}

#endif /* CLIENT_WORLD_CHUNK_QUAD_HH */
