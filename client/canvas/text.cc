// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <algorithm>
#include <client/canvas/text.hh>
#include <shared/math/constexpr.hh>

void Text::create(Text &text, int width, int height)
{
    text.texture_width = 1 << cxpr::log2(width);
    text.texture_height = 1 << cxpr::log2(height);

    // Initially the texture is filled with a special
    // character 0xFFFFFFFF. The (GLuint)(-1) is a not
    // valid unicode character so it can be used as a
    // character that is rendered as invisible
    std::vector<GLuint> filler_vec = {};
    filler_vec.resize(text.texture_width * text.texture_height * 3, 0xFFFFFFFF);

    if(!text.handle)
        glGenTextures(1, &text.handle);
    glBindTexture(GL_TEXTURE_2D, text.handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, text.texture_width, text.texture_height, 0, GL_RED, GL_UNSIGNED_INT, filler_vec.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // It's a good idea to reuse the same vector to avoid
    // per-frame heap allocations since Text::set might be
    // called every frame to render GUI and in-world labels
    text.cache.resize(text.texture_width);
}

void Text::set(Text &text, int line, const std::string &str)
{
    // The texture can hold just so much text in a single line
    // FIXME: implement something that would wrap the text around?
    const int max_size = cxpr::min<int>(str.size(), text.texture_width);

    // Initially the texture is filled with a special
    // character 0xFFFFFFFF. The (GLuint)(-1) is a not
    // valid unicode character so it can be used as a
    // character that is rendered as invisible
    std::fill(text.cache.begin(), text.cache.end(), 0xFFFFFFFF);

    for(int i = 0; i < max_size; ++i) {
        // NOTE: this assumes the input string contains
        // UTF-8 without sequences aka just pure ASCII.
        text.cache[i] = static_cast<GLuint>(str[i]);
    }

    if(max_size > text.max_text_width)
        text.max_text_width = max_size;
    if(line >= text.max_text_height)
        text.max_text_height = line + 1;

    glBindTexture(GL_TEXTURE_2D, text.handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, text.texture_height - line - 1, text.texture_width, 1, GL_RED, GL_UNSIGNED_INT, text.cache.data());
}

void Text::set(Text &text, int line, const std::wstring &str)
{
    // The texture can hold just so much text in a single line
    // FIXME: implement something that would wrap the text around?
    const int max_size = cxpr::min<int>(str.size(), text.texture_width);

    // Initially the texture is filled with a special
    // character 0xFFFFFFFF. The (GLuint)(-1) is a not
    // valid unicode character so it can be used as a
    // character that is rendered as invisible
    std::fill(text.cache.begin(), text.cache.end(), 0xFFFFFFFF);

    for(int i = 0; i < max_size; ++i) {
        // NOTE: this assumes wchar_t is either a 32-bit value
        // or the string in UTF-16 contains no surrogates
        text.cache[i] = static_cast<GLuint>(str[i]);
    }

    if(max_size > text.max_text_width)
        text.max_text_width = max_size;
    if(line >= text.max_text_height)
        text.max_text_height = line + 1;

    glBindTexture(GL_TEXTURE_2D, text.handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, text.texture_height - line - 1, text.texture_width, 1, GL_RED, GL_UNSIGNED_INT, text.cache.data());
}

void Text::destroy(Text &text)
{
    if(text.handle)
        glDeleteTextures(1, &text.handle);
    text.cache.clear();
    text.max_text_height = 0;
    text.max_text_width = 0;
    text.texture_height = 0;
    text.texture_width = 0;
    text.handle = 0;
}
