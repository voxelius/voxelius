// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/fonts.hh>
#include <client/image.hh>
#include <spdlog/spdlog.h>
#include <unordered_map>

static std::unordered_map<std::string, Font> map = {};

const Font *fonts::load_image(const std::string &name, const vfs::path_t &path, int width, int height)
{
    if(const auto it = map.find(name); it != map.cend()) {
        spdlog::warn("gui: {}: font exists", name);
        return &it->second;
    }

    Image image = {};
    Font font = {};

    if(image.load_grayscale(path, true)) {
        const int image_width = image.get_width();
        const int image_height = image.get_height();

        font.glyph_width = width;
        font.glyph_height = height;
        font.texture_cwidth = image_width / font.glyph_width;
        font.texture_cheight = image_height / font.glyph_height;

        font.texture.create();
        font.texture.storage(image_width, image_height, Image::GRAYSCALE);
        font.texture.write(0, 0, image_width, image_height, Image::GRAYSCALE, image.data());

        return &map.emplace(name, std::move(font)).first->second;
    }

    return nullptr;
}

const Font *fonts::load_rom(const std::string &name, const vfs::path_t &path, int width, int height)
{
    // UNDONE: BIOS ROM fonts
    // OpenGL appears to not support
    // loading textures with 1-bit depth...
    return nullptr;
}

const Font *fonts::load_psf(const std::string &name, const vfs::path_t &path, bool unicode)
{
    // UNDONE: PSF1/PSF2 fonts
    // OpenGL appears to not support
    // loading textures with 1-bit depth...
    return nullptr;
}

const Font *fonts::find(const std::string &name)
{
    const auto it = map.find(name);
    if(it != map.cend())
        return &it->second;
    return nullptr;
}

void fonts::purge()
{
    map.clear();
}
