// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/text_builder.hh>
#include <client/gui/text_vbo.hh>

void TextVBO::create(TextVBO &buffer, const TextBuilder &builder)
{
    if(!buffer.handle)
        glGenBuffers(1, &buffer.handle);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    glBufferData(GL_ARRAY_BUFFER, builder.quads.size() * sizeof(TextQuad), builder.quads.data(), GL_DYNAMIC_DRAW);
    buffer.size = builder.quads.size();
}

void TextVBO::destroy(TextVBO &buffer)
{
    if(buffer.handle)
        glDeleteBuffers(1, &buffer.handle);
    buffer.handle = 0;
    buffer.size = 0;
}
