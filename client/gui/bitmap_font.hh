// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_BITMAP_FONT_HH
#define CLIENT_GUI_BITMAP_FONT_HH
#include <cstdint>
#include <glad/gl.h>
#include <string>
#include <vector>

class BitmapFont final {
public:
    constexpr static int PAGE_SIZE = 16;

public:
    int glyph_height {};
    std::size_t num_pages {};
    std::vector<int> glyph_offsets {};
    std::vector<int> glyph_widths {};
    GLuint handle {};

public:
    static bool load(BitmapFont &font, const std::string &path);
    static void unload(BitmapFont &font);
};

#endif /* CLIENT_GUI_BITMAP_FONT_HH */
