// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CANVAS_TEXT_HH
#define CLIENT_CANVAS_TEXT_HH
#include <glad/gl.h>
#include <string>
#include <vector>

// This represents a generic text
// block stored in the GPU memory as a
// 2D texture filled with unicode characters
class Text final {
public:
    GLuint handle {};
    int texture_width {};
    int texture_height {};
    int max_text_width {};
    int max_text_height {};
    std::vector<GLuint> cache {};

public:
    static void create(Text &text, int width, int heighy);
    static void set(Text &text, int line, const std::string &str);
    static void set(Text &text, int line, const std::wstring &str);
    static void destroy(Text &Text);
};

#endif /* CLIENT_CANVAS_TEXT_HH */
