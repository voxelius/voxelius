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
    constexpr static int UPAGE_SIZE = 16;
    constexpr static int UPAGE_AREA = UPAGE_SIZE * UPAGE_SIZE;

public:
    int glyph_height {};
    std::size_t num_pages {};
    std::vector<int> glyph_widths {};
    GLuint handle {};

public:
    static bool load(BitmapFont &font, int height, const std::string &name);
    static void unload(BitmapFont &font);
};

#endif /* CLIENT_GUI_BITMAP_FONT_HH */
