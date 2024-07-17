// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#include <physfs.h>
#include <shared/image.hh>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <unordered_map>
#include <vector>

Image::~Image(void)
{
    unload();
}

Image::Image(const std::string &path, bool flip)
{
    this->width = 0;
    this->height = 0;
    this->pixels = nullptr;
    load_rgba(path, flip);
}

bool Image::load_gray(const std::string &path, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);

    unload();

    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        std::vector<std::uint8_t> buffer = {};

        buffer.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, buffer.data(), buffer.size());
        PHYSFS_close(file);

        const auto *uc_buffer = reinterpret_cast<const stbi_uc *>(buffer.data());
        pixels = stbi_load_from_memory(uc_buffer, buffer.size(), &width, &height, nullptr, STBI_grey);
        return width && height && pixels;
    }

    return false;
}

bool Image::load_rgba(const std::string &path, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);

    unload();

    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        std::vector<std::uint8_t> buffer = {};

        buffer.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, buffer.data(), buffer.size());
        PHYSFS_close(file);

        const auto *uc_buffer = reinterpret_cast<const stbi_uc *>(buffer.data());
        pixels = stbi_load_from_memory(uc_buffer, buffer.size(), &width, &height, nullptr, STBI_rgb_alpha);
        return width && height && pixels;
    }

    return false;
}

void Image::unload(void)
{
    if(pixels)
        stbi_image_free(pixels);
    pixels = nullptr;
    height = 0;
    width = 0;
}

bool Image::is_valid(void) const
{
    return width && height && pixels;
}

int Image::get_width(void) const
{
    return width;
}

int Image::get_height(void) const
{
    return height;
}

const void *Image::data(void) const
{
    return pixels;
}

void *Image::data(void)
{
    return pixels;
}
