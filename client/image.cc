// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <client/image.hh>
#include <physfs.h>
#include <stb_image.h>
#include <vector>

bool Image::load_gray(Image &image, const std::string &path, bool flip)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        std::vector<unsigned char> buffer = {};

        buffer.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, buffer.data(), buffer.size());
        PHYSFS_close(file);

        stbi_set_flip_vertically_on_load(flip);
        image.data = stbi_load_from_memory(buffer.data(), buffer.size(), &image.width, &image.height, nullptr, STBI_grey);
        return image.width && image.height && image.data;
    }

    return false;
}

bool Image::load_rgba(Image &image, const std::string &path, bool flip)
{
    if(PHYSFS_File *file = PHYSFS_openRead(path.c_str())) {
        std::vector<unsigned char> buffer = {};

        buffer.resize(PHYSFS_fileLength(file));
        PHYSFS_readBytes(file, buffer.data(), buffer.size());
        PHYSFS_close(file);

        stbi_set_flip_vertically_on_load(flip);
        image.data = stbi_load_from_memory(buffer.data(), buffer.size(), &image.width, &image.height, nullptr, STBI_rgb_alpha);
        return image.width && image.height && image.data;
    }

    return false;
}

void Image::unload(Image &image)
{
    image.width = 0;
    image.height = 0;
    stbi_image_free(image.data);
}
