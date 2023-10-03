// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/image.hh>
#include <stb_image.h>

Image::~Image()
{
    unload();
}

bool Image::load_grayscale(const vfs::path_t &path, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);

    unload();

    std::vector<uint8_t> buffer = {};
    if(vfs::read_bytes(path, buffer)) {
        const auto *buffer_p = reinterpret_cast<const stbi_uc *>(buffer.data());
        pixels = stbi_load_from_memory(buffer_p, static_cast<int>(buffer.size()), &width, &height, nullptr, STBI_grey);
        return width && height && pixels;
    }

    return false;
}

bool Image::load_rgba(const vfs::path_t &path, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);

    unload();

    std::vector<uint8_t> buffer = {};
    if(vfs::read_bytes(path, buffer)) {
        const auto *buffer_p = reinterpret_cast<const stbi_uc *>(buffer.data());
        pixels = stbi_load_from_memory(buffer_p, static_cast<int>(buffer.size()), &width, &height, nullptr, STBI_rgb_alpha);
        return width && height && pixels;
    }

    return false;
}

void Image::unload()
{
    if(pixels)
        stbi_image_free(pixels);
    width = 0;
    height = 0;
    pixels = nullptr;
}

bool Image::valid() const
{
    return width && height && pixels;
}

int Image::get_width() const
{
    return width;
}

int Image::get_height() const
{
    return height;
}

void Image::get_size(int &width, int &height) const
{
    width = this->width;
    height = this->height;
}

const void *Image::data() const
{
    return pixels;
}

void *Image::data()
{
    return pixels;
}
