// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_VBO_HH
#define CLIENT_GUI_TEXT_VBO_HH
#include <client/gui/text_quad.hh>
#include <glad/gl.h>

class TextVBO final {
public:
    std::size_t size {};
    GLuint handle {};

public:
    static void create(TextVBO &buffer, const TextQuad *data, std::size_t size);
    static void destroy(TextVBO &buffer);
};

#endif /* CLIENT_GUI_TEXT_VBO_HH */
