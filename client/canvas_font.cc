// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/image.hh>
#include <shared/cxmath.hh>

bool canvas::Font::load(const vfs::path_t &path, int width, int height)
{
    Image image = {};

    if(image.load_grayscale(path, true)) {
        const int image_width = image.get_width();
        const int image_height = image.get_height();

        glyph_width = width;
        glyph_height = height;
        texture_cwidth = image_width / width;
        texture_cheight = image_height / height;

        texture.create();
        texture.storage(image_width, image_height, Image::GRAYSCALE);
        texture.write(0, 0, image_width, image_height, Image::GRAYSCALE, image.data());

        return true;
    }

    return false;
}

void canvas::Font::unload()
{
    glyph_width = 0;
    glyph_height = 0;
    texture_cwidth = 0;
    texture_cheight = 0;
    texture.destroy();
}

int canvas::Font::get_glyph_width() const
{
    return glyph_width;
}

int canvas::Font::get_glyph_height() const
{
    return glyph_height;
}

int canvas::Font::get_texture_cwidth() const
{
    return texture_cwidth;
}

int canvas::Font::get_texture_cheight() const
{
    return texture_cheight;
}

const glxx::Texture2D &canvas::Font::get() const
{
    return texture;
}
