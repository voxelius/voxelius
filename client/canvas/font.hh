// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CANVAS_FONT_HH
#define CLIENT_CANVAS_FONT_HH
#include <glad/gl.h>
#include <string>

class Font final {
public:
    int glyph_width {};
    int glyph_height {};
    int texture_x_glyphs {};
    int texture_y_glyphs {};
    GLuint handle {};

public:
    static bool load_binary(Font &font, int width, int height, const std::string &path);
    static bool load_image(Font &font, int width, int height, const std::string &path);
    static void unload(Font &font);
};

#endif /* CLIENT_CANVAS_FONT_HH */
