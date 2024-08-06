// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_QUAD_HH
#define CLIENT_GUI_TEXT_QUAD_HH
#include <client/gui/text_attribute.hh>
#include <shared/math/constexpr.hh>
#include <shared/math/vec4f.hh>
#include <utility>

// [0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
// [1] TTTTTTTTUUUUUUUUUUUUUUUUUUUUUUUU
// [2] RRRRRRRRGGGGGGGGBBBBBBBBrrrrrrrr
// [3] ggggggggbbbbbbbbWWWWWWWWOOOOOOOO
using TextQuad = std::array<std::uint32_t, 4>;

constexpr static inline TextQuad make_text_quad(unsigned int xpos, unsigned int ypos, TextAttributeBits attributes, char32_t unicode, const Vec4f &background, const Vec4f &foreground, unsigned int width, unsigned int offset)
{
    TextQuad result = {};

    // [0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
    result[0] |= (0x0000FFFFU & static_cast<std::uint32_t>(xpos)) << 16U;
    result[0] |= (0x0000FFFFU & static_cast<std::uint32_t>(ypos)) << 0U;

    // [1] TTTTTTTTUUUUUUUUUUUUUUUUUUUUUUUU
    result[1] |= (0x000000FFU & static_cast<std::uint32_t>(attributes)) << 24U;
    result[1] |= (0x00FFFFFFU & static_cast<std::uint32_t>(unicode)) << 0U;

    // [2] RRRRRRRRGGGGGGGGBBBBBBBBrrrrrrrr
    // [3] ggggggggbbbbbbbb----------------
    result[2] |= (0x000000FFU & static_cast<std::uint32_t>(background[0] * 255.0f)) << 24U;
    result[2] |= (0x000000FFU & static_cast<std::uint32_t>(background[1] * 255.0f)) << 16U;
    result[2] |= (0x000000FFU & static_cast<std::uint32_t>(background[2] * 255.0f)) << 8U;

    result[2] |= (0x000000FFU & static_cast<std::uint32_t>(foreground[0] * 255.0f)) << 0U;
    result[3] |= (0x000000FFU & static_cast<std::uint32_t>(foreground[1] * 255.0f)) << 24U;
    result[3] |= (0x000000FFU & static_cast<std::uint32_t>(foreground[2] * 255.0f)) << 16U;

    // [3] ----------------WWWWWWWWOOOOOOOO
    result[3] |= (0x000000FFU & static_cast<std::uint32_t>(width)) << 8U;
    result[3] |= (0x000000FFU & static_cast<std::uint32_t>(offset)) << 0U;

    return std::move(result);
}

#endif /* CLIENT_GUI_TEXT_QUAD_HH */
