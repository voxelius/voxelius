// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_FONT_HH
#define CLIENT_FONT_HH
#include <glad/gl.h>
#include <schrift.h>
#include <string>
#include <vector>

class Font final {
public:
    // The font is rendered into 256-glyph pages;
    // each glyph is at most <height> pixels wide;
    constexpr static int PAGE_SIZE = 16;
    constexpr static char32_t ASCII = char32_t(0x000000FF);
    constexpr static char32_t PLANE0 = char32_t(0x0000FFFF);

public:
    int glyph_height {};
    std::size_t num_pages {};
    std::vector<SFT_GMetrics> gmetrics {};
    SFT_LMetrics lmetrics {};
    GLuint handle {};

public:
    static bool load(Font &font, char32_t maxcp, int height, const std::string &path);
    static void unload(Font &font);
};

#endif /* CLIENT_FONT_HH */
