// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_QUAD_VERTEX_HH
#define CLIENT_QUAD_VERTEX_HH
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <shared/util/cxmath.hh>
#include <shared/vdef.hh>

// [0] XXXXXXXXYYYYYYYYZZZZZZZZWWWWHHHH
// [1] FFFFTTTTTTTTTTTAAAAA------------
using QuadVertex = glm::vec<2, std::uint32_t>;

constexpr inline static QuadVertex make_quad_vertex(const glm::fvec3 &position, const glm::fvec2 &size, VoxelFacing facing, std::size_t texture, std::size_t frames)
{
    QuadVertex result = {};
    result[0] = 0x00000000;
    result[1] = 0x00000000;

    // [0] XXXXXXXXYYYYYYYYZZZZZZZZ--------
    // [1] --------------------------------
    result[0] |= util::min<std::uint32_t>(0x00FF, position.x * 16.0f) << 24;
    result[0] |= util::min<std::uint32_t>(0x00FF, position.y * 16.0f) << 16;
    result[0] |= util::min<std::uint32_t>(0x00FF, position.z * 16.0f) << 8;

    // [0] ------------------------WWWWHHHH
    // [1] --------------------------------
    result[0] |= util::min<std::uint32_t>(0x000F, size.x * 16.0f - 1.0f) << 4;
    result[0] |= util::min<std::uint32_t>(0x000F, size.y * 16.0f - 1.0f);

    // [0] --------------------------------
    // [1] FFFF----------------------------
    result[1] |= util::min<std::uint32_t>(0x000F, facing) << 28;

    // [0] --------------------------------
    // [1] ----TTTTTTTTTTTAAAAA------------
    result[1] |= util::min<std::uint32_t>(0x07FF, texture) << 17;
    result[1] |= util::min<std::uint32_t>(0x001F, frames) << 12;

    return std::move(result);
}

#endif /* CLIENT_QUAD_VERTEX_HH */
