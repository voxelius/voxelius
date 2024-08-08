// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_TEXT_QUAD_HH
#define CLIENT_TEXT_QUAD_HH
#include <client/text_attribute.hh>
#include <schrift.h>
#include <shared/math/constexpr.hh>
#include <shared/math/floathacks.hh>
#include <shared/math/vec4f.hh>
#include <utility>

struct TextQuad final {
    // [0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
    // [1] TTTTTTTTUUUUUUUUUUUUUUUUUUUUUUUU
    // [2] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA (background)
    // [3] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA (foreground)
    std::array<std::uint32_t, 4> data_1 {};

    // [0] aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa (float bits)
    // [1] dddddddddddddddddddddddddddddddd (float bits)
    // [2] AAAAAAAAAAAAAAAAOOOOOOOOOOOOOOOO
    // [3] --------------------------------
    std::array<std::uint32_t, 4> data_2 {};
};

static inline TextQuad make_text_quad(int xpos, int ypos, TextAttributeBits attributes, char32_t unicode, const Vec4f &background, const Vec4f &foreground, const SFT_LMetrics &lmetrics, const SFT_GMetrics &gmetrics)
{
    TextQuad result = {};

    result.data_1[0] = 0x00000000;
    result.data_1[1] = 0x00000000;
    result.data_1[2] = 0x00000000;
    result.data_1[3] = 0x00000000;

    result.data_2[0] = 0x00000000;
    result.data_2[1] = 0x00000000;
    result.data_2[2] = 0x00000000;
    result.data_2[3] = 0x00000000;

    // data_1[0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
    result.data_1[0] |= (0x0000FFFFU & static_cast<std::uint32_t>(xpos + 0x8000)) << 16U;
    result.data_1[0] |= (0x0000FFFFU & static_cast<std::uint32_t>(ypos + 0x8000)) << 0U;

    // data_1[1] TTTTTTTTUUUUUUUUUUUUUUUUUUUUUUUU
    result.data_1[1] |= (0x000000FFU & static_cast<std::uint32_t>(attributes)) << 24U;
    result.data_1[1] |= (0x00FFFFFFU & static_cast<std::uint32_t>(unicode)) << 0U;

    // data_1[2] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA
    result.data_1[2] |= (0x000000FFU & static_cast<std::uint32_t>(background[0] * 255.0f)) << 24U;
    result.data_1[2] |= (0x000000FFU & static_cast<std::uint32_t>(background[1] * 255.0f)) << 16U;
    result.data_1[2] |= (0x000000FFU & static_cast<std::uint32_t>(background[2] * 255.0f)) << 8U;
    result.data_1[2] |= (0x000000FFU & static_cast<std::uint32_t>(background[3] * 255.0f)) << 0U;

    // data_1[3] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA
    result.data_1[3] |= (0x000000FFU & static_cast<std::uint32_t>(foreground[0] * 255.0f)) << 24U;
    result.data_1[3] |= (0x000000FFU & static_cast<std::uint32_t>(foreground[1] * 255.0f)) << 16U;
    result.data_1[3] |= (0x000000FFU & static_cast<std::uint32_t>(foreground[2] * 255.0f)) << 8U;
    result.data_1[3] |= (0x000000FFU & static_cast<std::uint32_t>(foreground[3] * 255.0f)) << 0U;

    // [0] aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa (float bits)
    result.data_2[0] |= floathacks::float_to_uint32(lmetrics.ascender);

    // [1] dddddddddddddddddddddddddddddddd (float bits)
    result.data_2[1] |= floathacks::float_to_uint32(lmetrics.descender);

    // [2] AAAAAAAAAAAAAAAAOOOOOOOOOOOOOOOO
    result.data_2[2] |= (0x0000FFFFU & static_cast<std::uint32_t>(static_cast<int>(gmetrics.advanceWidth) + 0x8000)) << 16U;
    result.data_2[2] |= (0x0000FFFFU & static_cast<std::uint32_t>(static_cast<int>(gmetrics.yOffset) + 0x8000)) << 0U;

    return std::move(result);
}

#endif /* CLIENT_TEXT_QUAD_HH */
