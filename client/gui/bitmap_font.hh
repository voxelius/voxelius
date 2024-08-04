// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_BITMAP_FONT_HH
#define CLIENT_GUI_BITMAP_FONT_HH
#include <cstdint>
#include <glad/gl.h>
#include <string>
#include <vector>

#if 0
class BitmapFont final {
public:
    constexpr static int UPAGE_SIZE = 16;

public:
    std::size_t num_pages {};
    std::uint8_t glyph_height {};
    std::vector<std::uint8_t> glyph_widths {};
    GLuint handle {};

public:
    static bool load(BitmapFont &font, const std::string &name);
    static void unload(BitmapFont &font);
};
#else
class BitmapFont final {
public:
    GLuint handle {};
    int glyph_width {};
    int glyph_height {};
    int num_glyphs_x {};
    int num_glyphs_y {};

public:
    static bool load(BitmapFont &font, int width, int height, const std::string &path);
    static void unload(BitmapFont &font);
};
#endif

#endif /* CLIENT_GUI_BITMAP_FONT_HH */
