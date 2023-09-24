// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/image.hh>
#include <shared/filetools.hh>
#include <spdlog/spdlog.h>
#include <stb_image.h>

Image::Image(const std::string &path)
{
    create(path);
}

Image::Image(Image &&rhs)
{
    std::swap(rhs.width, width);
    std::swap(rhs.height, height);
    std::swap(rhs.pixels, pixels);
}

Image::~Image()
{
    destroy();
}

Image &Image::operator=(Image &&rhs)
{
    Image copy = Image{std::move(rhs)};
    std::swap(copy.width, width);
    std::swap(copy.height, height);
    std::swap(copy.pixels, pixels);
    return (*this);
}

bool Image::create(const std::string &path)
{
    stbi_set_flip_vertically_on_load(true);

    destroy();

    std::vector<uint8_t> buffer = {};
    if(util::read(path, buffer)) {
        const auto *buffer_p = reinterpret_cast<const stbi_uc *>(buffer.data());
        pixels = stbi_load_from_memory(buffer_p, static_cast<int>(buffer.size()), &width, &height, nullptr, STBI_rgb_alpha);
        return width && height && pixels;
    }

    return false;
}

void Image::destroy()
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
