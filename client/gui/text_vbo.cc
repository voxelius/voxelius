// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/text_vbo.hh>

void TextVBO::create(TextVBO &text, const TextQuad *data, std::size_t size)
{
    if(!text.handle)
        glGenBuffers(1, &text.handle);
    glBindBuffer(GL_ARRAY_BUFFER, text.handle);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(TextQuad), data, GL_DYNAMIC_DRAW);
    text.size = size;
}

void TextVBO::destroy(TextVBO &text)
{
    if(text.handle)
        glDeleteBuffers(1, &text.handle);
    text.handle = 0;
    text.size = 0;
}
