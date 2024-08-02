// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_BUILDER_HH
#define CLIENT_GUI_TEXT_BUILDER_HH
#include <client/gui/text_mode.hh>
#include <client/gui/text_quad.hh>
#include <vector>
#include <string>

class TextBuilder final {
public:
    TextModeBits mode {};
    std::vector<TextQuad> quads {};

public:
    Vec4f background {};
    Vec4f foreground {};
    TextAttributeBits attributes {};

public:
    unsigned int cursor_x {};
    unsigned int cursor_y {};

public:
    static void append(TextBuilder &builder, const std::string &text);
    static void clear(TextBuilder &builder);
};

#endif /* CLIENT_GUI_TEXT_BUILDER_HH */
