// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_BITMAP_FONT_HH
#define CLIENT_GUI_BITMAP_FONT_HH
#include <glad/gl.h>
#include <shared/math/vec2f.hh>
#include <string>

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

#endif /* CLIENT_GUI_BITMAP_FONT_HH */
