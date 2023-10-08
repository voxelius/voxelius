// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/image.hh>
#include <client/ui_font.hh>
#include <shared/cxmath.hh>
#include <unordered_map>

bool ui::Font::load_image(const vfs::path_t &path, int width, int height)
{
    Image image = {};

    if(image.load_grayscale(path, true)) {
        const int image_width = image.get_width();
        const int image_height = image.get_height();
        const int pod_width = 1 << cxmath::log2(image_width);
        const int pod_height = 1 << cxmath::log2(image_height);

        glyph_width = width;
        glyph_height = height;
        texture_cwidth = image_width / width;
        texture_cheight = image_height / height;

        texture.create();
        texture.storage(pod_width, pod_height, Image::GRAYSCALE);
        texture.write(0, 0, image_width, image_height, Image::GRAYSCALE, image.data());

        return true;
    }

    return false;
}

bool ui::Font::load_vga_rom(const vfs::path_t &path, int width, int height)
{
    std::vector<uint8_t> rom = {};

    if(vfs::read_bytes(path, rom)) {
        const size_t scanline = ((width + 0x07) & ~0x07) / 8;
        const size_t stride = scanline * height;
        const size_t count = rom.size() / stride;

        glyph_width = width;
        glyph_height = height;
        texture_cwidth = cxmath::min<int>(count, 64);
        texture_cheight = cxmath::max<int>(1, count / 64);

        const int pod_width = 1 << cxmath::log2(glyph_width * texture_cwidth);
        const int pod_height = 1 << cxmath::log2(glyph_height * texture_cheight);

        texture.create();
        texture.storage(pod_width, pod_height, PixelFormat::R8_UNORM);

        std::vector<uint8_t> glyph = {};
        glyph.resize(glyph_width * glyph_height);

        for(size_t i = 0; i < count; ++i) {
            const uint8_t *gptr = &rom[i * stride];
            for(int gy = (glyph_height - 1); gy >= 0; --gy) {
                for(int gx = 0; gx < glyph_width; ++gx)
                    glyph[gy * glyph_width + gx] = (gptr[gx >> 3] & (0x80 >> (gx & 7))) ? 0xFF : 0x00;
                gptr += scanline;
            }

            const int xoff = glyph_width * (i % texture_cwidth);
            const int yoff = pod_height - glyph_height * (i / texture_cwidth) - glyph_height;
            texture.write(xoff, yoff, glyph_width, glyph_height, PixelFormat::R8_UNORM, glyph.data());
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
