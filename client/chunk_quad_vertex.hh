// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CHUNK_QUAD_VERTEX_HH
#define CLIENT_CHUNK_QUAD_VERTEX_HH
#include <shared/math/constexpr.hh>
#include <shared/math/vec2f.hh>
#include <shared/math/vec3f.hh>
#include <shared/vdef.hh>
#include <utility>

// [0] XXXXXXXXYYYYYYYYZZZZZZZZWWWWHHHH
// [1] FFFFTTTTTTTTTTTAAAAA------------
using ChunkQuadVertex = std::array<std::uint32_t, 2>;

constexpr inline static ChunkQuadVertex make_chunk_quad(const Vec3f &position, const Vec2f &size, VoxelFacing facing, std::size_t texture, std::size_t frames)
{
    ChunkQuadVertex result = {};
    result[0] = 0x00000000;
    result[1] = 0x00000000;

    // [0] XXXXXXXXYYYYYYYYZZZZZZZZ--------
    result[0] |= cxpr::min<std::uint32_t>(0x00FF, position[0] * 16.0f) << 24;
    result[0] |= cxpr::min<std::uint32_t>(0x00FF, position[1] * 16.0f) << 16;
    result[0] |= cxpr::min<std::uint32_t>(0x00FF, position[2] * 16.0f) << 8;

    // [0] ------------------------WWWWHHHH
    result[0] |= cxpr::min<std::uint32_t>(0x000F, size[0] * 16.0f - 1.0f) << 4;
    result[0] |= cxpr::min<std::uint32_t>(0x000F, size[1] * 16.0f - 1.0f);

    // [1] FFFF----------------------------
    result[1] |= cxpr::min<std::uint32_t>(0x000F, facing) << 28;

    // [1] ----TTTTTTTTTTTAAAAA------------
    result[1] |= cxpr::min<std::uint32_t>(0x07FF, texture) << 17;
    result[1] |= cxpr::min<std::uint32_t>(0x001F, frames) << 12;

    return std::move(result);
}

#endif /* CLIENT_CHUNK_QUAD_VERTEX_HH */
