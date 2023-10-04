// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/ui_label.hh>
#include <vector>

void ui::Label::set(const std::string &text)
{
    std::vector<uint32_t> pixels = {};
    const size_t last_size = text_size;

    destroy();

    text_size = text.size();
    pixels.resize(text_size);

    for(unsigned int i = 0; i < text_size; ++i) {
        // FIXME: this assumes std::string is
        // encoded as ASCII or UTF-8 without any
        // byte sequences apart from 7 bits
        pixels[i] = static_cast<uint32_t>(text[i]);
    }

    if(!texture.get() || (text_size > last_size)) {
        texture.create();
        texture.storage(text_size, 1, PixelFormat::R32_UINT);
    }

    texture.write(0, 0, text_size, 1, PixelFormat::R32_UINT, pixels.data());
}

void ui::Label::set(const std::wstring &text)
{
    std::vector<uint32_t> pixels = {};
    const size_t last_size = text_size;

    destroy();

    text_size = text.size();
    pixels.resize(text_size);

    for(unsigned int i = 0; i < text_size; ++i) {
        // FIXME: this assumes std::wstring is
        // either encoded as UTF-32 or as UTF-16
        // without surrogates. This is potentially
        // bad for, say a whole plane of CJK characters
        pixels[i] = static_cast<uint32_t>(text[i]);
    }

    if(!texture.get() || (text_size > last_size)) {
        texture.create();
        texture.storage(text_size, 1, PixelFormat::R32_UINT);
    }

    texture.write(0, 0, text_size, 1, PixelFormat::R32_UINT, pixels.data());
}

void ui::Label::destroy()
{
    texture.destroy();
    text_size = 0;
}

const gl::Texture2D &ui::Label::get() const
{
    return texture;
}

const size_t ui::Label::get_size() const
{
    return text_size;
}
