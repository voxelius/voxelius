// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CANVAS_TEXT_HH
#define CLIENT_CANVAS_TEXT_HH
#include <cstdint>
#include <glad/gl.h>
#include <string>
#include <vector>

class Text final {
public:
    GLuint handle {};
    int texture_width {};
    int texture_height {};
    int max_text_width {};
    int max_text_height {};

    // Re-using the vector object in Text::set
    // calls allows for much less memory allocations
    std::vector<std::uint32_t> storage {};

public:
    static void create(Text &text, int width, int height);
    static void set(Text &text, int line, const std::string &str);
    static void set(Text &text, int line, const std::wstring &str);
    static void destroy(Text &text);
};

#endif /* CLIENT_CANVAS_TEXT_HH */
