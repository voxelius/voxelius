// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <algorithm>
#include <client/canvas/text.hh>
#include <shared/math/constexpr.hh>
#include <vector>

void Text::create(Text &text, int width, int height)
{
    Text::destroy(text);

    text.texture_width = 1 << cxpr::log2(width);
    text.texture_height = 1 << cxpr::log2(height);

    // 0xFFFFFFFF is a special character that signals the
    // shader that this specific character place is not
    // supposed to be visible causing the fragment shader to discard
    std::vector<std::uint32_t> contents = {};
    contents.resize(text.texture_width, 0xFFFFFFFF);

    glGenTextures(1, &text.handle);
    glBindTexture(GL_TEXTURE_2D, text.handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, text.texture_width, text.texture_height, 0, GL_RED, GL_UNSIGNED_INT, contents.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    text.storage.resize(text.texture_width);
}

void Text::set(Text &text, int line, const std::string &str)
{
    // The Text object contains up to <size> characters
    const int size = cxpr::min<int>(str.size(), text.texture_width);

    // 0xFFFFFFFF is a special character that signals the
    // shader that this specific character place is not
    // supposed to be visible causing the fragment shader to discard
    std::fill(text.storage.begin(), text.storage.end(), 0xFFFFFFFF);

    for(int i = 0; i < size; ++i) {
        // NOTE: this assumes the input string contains
        // UTF-16 without surrogates or just pure UTF-32.
        text.storage[i] = static_cast<std::uint32_t>(str[i]);
    }

    if(size > text.max_text_width)
        text.max_text_width = size;
    if(line >= text.max_text_height)
        text.max_text_height = line + 1;

    glBindTexture(GL_TEXTURE_2D, text.handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, text.texture_height - line - 1, text.texture_width, 1, GL_RED, GL_UNSIGNED_INT, text.storage.data());
}

void Text::set(Text &text, int line, const std::wstring &str)
{
    // The Text object contains up to <size> characters
    const int size = cxpr::min<int>(str.size(), text.texture_width);

    // 0xFFFFFFFF is a special character that signals the
    // shader that this specific character place is not
    // supposed to be visible causing the fragment shader to discard
    std::fill(text.storage.begin(), text.storage.end(), 0xFFFFFFFF);

    for(int i = 0; i < size; ++i) {
        // NOTE: this assumes the input string contains
        // UTF-16 without surrogates or just pure UTF-32.
        text.storage[i] = static_cast<std::uint32_t>(str[i]);
    }

    if(size > text.max_text_width)
        text.max_text_width = size;
    if(line >= text.max_text_height)
        text.max_text_height = line + 1;

    glBindTexture(GL_TEXTURE_2D, text.handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, text.texture_height - line - 1, text.texture_width, 1, GL_RED, GL_UNSIGNED_INT, text.storage.data());
}

void Text::destroy(Text &text)
{
    if(text.handle)
        glDeleteTextures(1, &text.handle);
    text.storage.clear();
    text.max_text_height = 0;
    text.max_text_width = 0;
    text.texture_height = 0;
    text.texture_width = 0;
    text.handle = 0;
}
