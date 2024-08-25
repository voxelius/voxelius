// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/fstools.hh>
#include <common/image.hh>
#include <spdlog/spdlog.h>
#include <stb_image.h>

bool Image::load_gray(Image &image, const std::string &path, bool flip)
{
    Image::unload(image);

    std::vector<std::uint8_t> buffer = {};

    if(!fstools::read_bytes(path, buffer)) {
        spdlog::error("image: {}: {}", path, fstools::error());
        return false;
    }

    stbi_set_flip_vertically_on_load(flip);

    const auto buffer_len = static_cast<int>(buffer.size());
    const auto *buffer_uc = reinterpret_cast<const stbi_uc *>(buffer.data());

    image.pixels = stbi_load_from_memory(buffer_uc, buffer_len, &image.width, &image.height, nullptr, STBI_grey);

    return image.width && image.height && image.pixels;
}

bool Image::load_rgba(Image &image, const std::string &path, bool flip)
{
    Image::unload(image);

    std::vector<std::uint8_t> buffer = {};

    if(!fstools::read_bytes(path, buffer)) {
        spdlog::error("image: {}: {}", path, fstools::error());
        return false;
    }

    stbi_set_flip_vertically_on_load(flip);

    const auto buffer_len = static_cast<int>(buffer.size());
    const auto *buffer_uc = reinterpret_cast<const stbi_uc *>(buffer.data());

    image.pixels = stbi_load_from_memory(buffer_uc, buffer_len, &image.width, &image.height, nullptr, STBI_rgb_alpha);

    return image.width && image.height && image.pixels;
}

void Image::unload(Image &image)
{
    if(image.pixels)
        stbi_image_free(image.pixels);
    image.pixels = nullptr;
    image.height = 0;
    image.width = 0;
}
