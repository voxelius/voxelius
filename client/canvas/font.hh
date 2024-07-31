// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CANVAS_FONT_HH
#define CLIENT_CANVAS_FONT_HH
#include <glad/gl.h>
#include <string>

class Font final {
public:
    GLuint handle {};
    int glyph_width {};
    int glyph_height {};
    int texture_cwidth {};
    int texture_cheight {};

public:
    static bool load(Font &font, int glyph_width, int glyph_height, const std::string &path);
    static void unload(Font &font);
};

#endif /* CLIENT_CANVAS_FONT_HH */
