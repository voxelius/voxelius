// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/image.hh>
#include <client/ui/font.hh>
#include <spdlog/spdlog.h>

bool ui::Font::load_image(const vfs::path_t &path, int width, int height)
{
    Image image = {};

    if(image.load_grayscale(path, true)) {
        const int iwidth = 1 << cxmath::log2(image.get_width());
        const int iheight = 1 << cxmath::log2(image.get_height());

        glyph_width = width;
        glyph_height = height;
        texture_cwidth = iwidth / width;
        texture_cheight = iheight / height;

        texture.create();
        texture.storage(iwidth, iheight, Image::GRAYSCALE);
        texture.write(0, 0, iwidth, iheight, Image::GRAYSCALE, image.data());

        return true;
    }

    return false;
}

bool ui::Font::load_rom(const vfs::path_t &path, int width, int height)
{
    std::vector<uint8_t> file = {};

    if(vfs::read_bytes(path, file)) {
        const size_t max_cwidth = 64;
        const size_t scanline = ((width + 0x07) & ~0x07) / 8;
        const size_t stride = scanline * height;
        const size_t count = file.size() / stride;

        glyph_width = width;
        glyph_height = height;
        texture_cwidth = cxmath::min<size_t>(count, max_cwidth);
        texture_cheight = cxmath::max<size_t>(1, count / max_cwidth);

        const int iwidth = 1 << cxmath::log2(glyph_width * texture_cwidth);
        const int iheight = 1 << cxmath::log2(glyph_height * texture_cheight);

        std::vector<uint8_t> glyph = {};
        glyph.resize(glyph_width * glyph_height);

        texture.create();
        texture.storage(iwidth, iheight, PixelFormat::R8_UNORM);

        for(size_t i = 0; i < count; ++i) {
            const uint8_t *gptr = &file.at(i * stride);

            // Copied straight from KanOS
            for(int gy = (glyph_height - 1); gy >= 0; --gy) {
                for(int gx = 0; gx < glyph_width; ++gx)
                    glyph[gy * glyph_width + gx] = (gptr[gx >> 3] & (0x80 >> (gx & 7))) ? 0xFF : 0x00;
                gptr += scanline;
            }

            const int xpos = glyph_width * (i % texture_cwidth);
            const int ypos = iheight - glyph_height * (i / texture_cwidth) - glyph_height;
            texture.write(xpos, ypos, glyph_width, glyph_height, PixelFormat::R8_UNORM, glyph.data());
        }

        return true;
    }

    return false;
}

void ui::Font::unload()
{
    glyph_width = 0;
    glyph_height = 0;
    texture_cwidth = 0;
    texture_cheight = 0;
    texture.destroy();
}

int ui::Font::get_glyph_width() const
{
    return glyph_width;
}

int ui::Font::get_glyph_height() const
{
    return glyph_height;
}

int ui::Font::get_texture_cwidth() const
{
    return texture_cwidth;
}

int ui::Font::get_texture_cheight() const
{
    return texture_cheight;
}

const gl::Texture2D &ui::Font::get() const
{
    return texture;
}

