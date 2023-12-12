// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <algorithm>
#include <client/canvas_text.hh>
#include <vector>

void canvas::Text::create(int width, int height)
{
    texture_width = 1 << cxmath::log2(width);
    texture_height = 1 << cxmath::log2(height);

    // We need to fill the texture up
    // with 0xFFFFFFFF which is a magic
    // constant for an invisible character
    std::vector<uint32_t> pixels = {};
    pixels.resize(texture_width * texture_height, 0xFFFFFFFF);

    texture.create();
    texture.storage(texture_width, texture_height, PixelFormat::R32_UINT);
    texture.write(0, 0, texture_width, texture_height, PixelFormat::R32_UINT, pixels.data());

    vec.resize(texture_width);
}

void canvas::Text::set(int line, const std::string &value)
{
    // We only can write up to <width> characters
    const int size = cxmath::min<int>(value.size(), texture_width);

    std::fill(vec.begin(), vec.end(), 0xFFFFFFFF);

    for(int i = 0; i < size; ++i) {
        // NOTE: this assumes the input string contains
        // UTF-8 without sequences aka just pure ASCII.
        vec[i] = static_cast<uint32_t>(value[i]);
    }

    if(size > max_text_width)
        max_text_width = size;
    if(line >= max_text_height)
        max_text_height = line + 1;
    texture.write(0, texture_height - line - 1, texture_width, 1, PixelFormat::R32_UINT, vec.data());
}

void canvas::Text::set(int line, const std::wstring &value)
{
    // We only can write up to <width> characters
    const int size = cxmath::min<int>(value.size(), texture_width);

    std::fill(vec.begin(), vec.end(), 0xFFFFFFFF);

    for(int i = 0; i < size; ++i) {
        // NOTE: this assumes the input string contains
        // UTF-16 without surrogates or just pure UTF-32.
        vec[i] = static_cast<uint32_t>(value[i]);
    }

    if(size > max_text_width)
        max_text_width = size;
    if(line >= max_text_height)
        max_text_height = line + 1;
    texture.write(0, texture_height - line - 1, texture_width, 1, PixelFormat::R32_UINT, vec.data());
}

void canvas::Text::destroy()
{
    max_text_width = 0;
    max_text_height = 0;
    texture_width = 0;
    texture_height = 0;
    texture.destroy();
}

int canvas::Text::get_max_text_width() const
{
    return max_text_width;
}

int canvas::Text::get_max_text_height() const
{
    return max_text_height;
}

int canvas::Text::get_texture_width() const
{
    return texture_width;
}

int canvas::Text::get_texture_height() const
{
    return texture_height;
}

const glxx::Texture2D &canvas::Text::get() const
{
    return texture;
}
