// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_TEXT_VBO_HH
#define CLIENT_TEXT_VBO_HH
#include <cstddef>
#include <glad/gl.h>

class TextBuilder;
class TextVBO final {
public:
    std::size_t size {};
    GLuint handle {};

public:
    static void create(TextVBO &buffer, const TextBuilder &builder);
    static void destroy(TextVBO &buffer);
};

#endif /* CLIENT_TEXT_VBO_HH */
