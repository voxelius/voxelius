// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_TEXT_BUILDER_HH
#define CLIENT_TEXT_BUILDER_HH
#include <client/text_mode.hh>
#include <client/text_quad.hh>
#include <vector>
#include <string>

class Font;
class TextBuilder final {
public:
    TextModeBits mode {};
    std::vector<TextQuad> quads {};

public:
    Vec4f background {};
    Vec4f foreground {};
    TextAttributeBits attributes {};

public:
    int cursor_x {};
    int cursor_y {};

public:
    static void append(TextBuilder &builder, const Font &font, const std::string &text);
    static void clear(TextBuilder &builder);
};

#endif /* CLIENT_TEXT_BUILDER_HH */
