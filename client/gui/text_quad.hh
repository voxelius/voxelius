// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_QUAD_VERTEX_HH
#define CLIENT_GUI_TEXT_QUAD_VERTEX_HH
#include <cstdint>
#include <shared/math/constexpr.hh>
#include <shared/math/vec2f.hh>
#include <shared/math/vec4f.hh>
#include <utility>

// [0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
// [1] CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
// [2] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA (FG)
// [3] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA (BG)
using TextQuad = std::array<std::uint32_t, 4>;

constexpr static inline TextQuad make_text_quad(unsigned int cx, unsigned int cy, wchar_t value, const Vec4f &fg, const Vec4f &bg)
{
    TextQuad result = {};
    result[0] = 0x00000000;
    result[1] = 0x00000000;
    result[2] = 0x00000000;
    result[3] = 0x00000000;

    // [0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
    result[0] |= cxpr::min<std::uint32_t>(0x0000FFFF, cx) << 16;
    result[0] |= cxpr::min<std::uint32_t>(0x0000FFFF, cy);

    // [1] CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    result[1] |= cxpr::min<std::uint32_t>(0xFFFFFFFF, value);

    // [2] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA (FG)
    result[2] |= cxpr::min<std::uint32_t>(0x000000FF, fg[0] * 255.0f) << 24;
    result[2] |= cxpr::min<std::uint32_t>(0x000000FF, fg[1] * 255.0f) << 16;
    result[2] |= cxpr::min<std::uint32_t>(0x000000FF, fg[2] * 255.0f) << 8;
    result[2] |= cxpr::min<std::uint32_t>(0x000000FF, fg[3] * 255.0f);

    // [3] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA (BG)
    result[3] |= cxpr::min<std::uint32_t>(0x000000FF, bg[0] * 255.0f) << 24;
    result[3] |= cxpr::min<std::uint32_t>(0x000000FF, bg[1] * 255.0f) << 16;
    result[3] |= cxpr::min<std::uint32_t>(0x000000FF, bg[2] * 255.0f) << 8;
    result[3] |= cxpr::min<std::uint32_t>(0x000000FF, bg[3] * 255.0f);

    return std::move(result);
}

#endif /* CLIENT_GUI_TEXT_QUAD_VERTEX_HH */
