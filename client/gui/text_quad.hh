// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_QUAD_VERTEX_HH
#define CLIENT_GUI_TEXT_QUAD_VERTEX_HH
#include <array>
#include <cstdint>
#include <shared/math/constexpr.hh>
#include <utility>

/*
BitmapFont font = {};
BitmapFont::load(font, 8, 16, "textures/fonts/unscii_8x16.png");

TextBuilder builder_a = {};
TextBuilder::set_cap(builder_a, TextCap::ASCII, true);
TextBuilder::set_cap(builder_a, TextCap::VT100, false);
TextBuilder::append(builder_a, "This text is supposed to use uniform colors");
TextBuilder::append(builder_a, "The color for each character is calculated as");
TextBuilder::append(builder_a, " a product of uniform and per-character colors;");
TextBuilder::append(builder_a, "Setting VT100 cap to false defaults per-character color to all white");
TextBuilder::append(builder_a, " allowing text renderer to decide what colors to use");

TextBuilder builder_b = {};
TextBuilder::set_cap(builder_b, TextCap::ASCII, true);
TextBuilder::set_cap(builder_b, TextCap::VT100, true);
TextBuilder::append(builder_b, "However, when the VT100 cap is set to \e[92mtrue\e[m, things are different");
TextBuilder::append(builder_b, "The text renderer ignores any uniform colors and instead uses all white");
TextBuilder::append(builder_b, " allowing the VT100-capable text to set it's own colors");
TextBuilder::append(builder_b, "The VT100 sequence parser state is reset for every append() call;");
TextBuilder::append(builder_b, "this means that text that has line feed characters retains set colors");

TextVBO text_a = {};
TextVBO::create(text_a, builder_a);
TextVBO::get_cap(text_a, TextCap::ASCII); // true
TextVBO::get_cap(text_a, TextCap::VT100); // false

text_renderer::prepare();
text_renderer::set_foreground(Vec4f::light_gray());
text_renderer::set_background(Vec4f::black(0.50f));
text_renderer::draw(16, 16, text_a, font);

TextVBO text_b = {};
TextVBO::create(text_b, builder_b);
TextVBO::get_cap(text_b, TextCap::ASCII); // true
TextVBO::get_cap(text_b, TextCap::VT100); // true

text_renderer::prepare();
text_renderer::set_foreground(Vec4f::yellow()); // ignored - TextCap::VT100
text_renderer::set_foreground(Vec4f::blue()); // ignored - TextCap::VT100
text_renderer::draw(16, 16, text_b, font);

TextVBO::destroy(text_b);
TextVBO::destroy(text_a);
BitmapFont::unload(font);
*/






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
