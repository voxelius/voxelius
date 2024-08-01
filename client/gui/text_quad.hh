// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_QUAD_VERTEX_HH
#define CLIENT_GUI_TEXT_QUAD_VERTEX_HH
#include <array>
#include <cstdint>
#include <shared/math/constexpr.hh>
#include <utility>

// [0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
// [1] CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
using TextQuad = std::array<std::uint32_t, 2>;

constexpr static inline TextQuad make_text_quad(unsigned int cx, unsigned int cy, char32_t unicode)
{
    TextQuad result = {};
    result[0] = 0x00000000;
    result[1] = 0x00000000;
    result[2] = 0x00000000;
    result[3] = 0x00000000;

    // [0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
    result[0] |= (0x0000FFFF & static_cast<std::uint32_t>(cx)) << 16;
    result[0] |= (0x0000FFFF & static_cast<std::uint32_t>(cy));

    // [1] CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    result[1] |= (0xFFFFFFFF & static_cast<std::uint32_t>(unicode));

    return std::move(result);
}

#endif /* CLIENT_GUI_TEXT_QUAD_VERTEX_HH */
