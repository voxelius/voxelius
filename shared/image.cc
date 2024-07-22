// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/util/physfs.hh>
#include <shared/image.hh>
#include <spdlog/spdlog.h>
#include <stb_image.h>

bool Image::load_gray(Image &image, const std::string &path, bool flip)
{
    Image::unload(image);

    std::vector<std::uint8_t> buffer = {};

    if(!util::read_bytes(path, buffer)) {
        spdlog::error("image: {}: {}", path, util::physfs_error());
        return false;
    }

    stbi_set_flip_vertically_on_load(flip);

    const auto buffer_len = static_cast<int>(buffer.size());
    const auto *buffer_uc = reinterpret_cast<const stbi_uc *>(buffer.data());

    image.pixels = stbi_load_from_memory(buffer_uc, buffer_len,
        &image.size.x, &image.size.y, nullptr, STBI_grey);
    return image.size.x && image.size.y && image.pixels;
}

bool Image::load_rgba(Image &image, const std::string &path, bool flip)
{
    Image::unload(image);

    std::vector<std::uint8_t> buffer = {};

    if(!util::read_bytes(path, buffer)) {
        spdlog::error("image: {}: {}", path, util::physfs_error());
        return false;
    }

    stbi_set_flip_vertically_on_load(flip);

    const auto buffer_len = static_cast<int>(buffer.size());
    const auto *buffer_uc = reinterpret_cast<const stbi_uc *>(buffer.data());

    image.pixels = stbi_load_from_memory(buffer_uc, buffer_len,
        &image.size.x, &image.size.y, nullptr, STBI_rgb_alpha);
    return image.size.x && image.size.y && image.pixels;
}

void Image::unload(Image &image)
{
    if(image.pixels)
        stbi_image_free(image.pixels);
    image.pixels = nullptr;
    image.size.x = 0;
    image.size.y = 0;
}
