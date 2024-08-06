// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/bitmap_font.hh>
#include <shared/math/constexpr.hh>
#include <shared/util/physfs.hh>
#include <shared/image.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

bool BitmapFont::load(BitmapFont &font, int height, const std::string &name)
{
    const std::string blob_path = fmt::format("fonts/{}.glyph_sizes.bin", name);
    std::vector<std::uint8_t> widths_blob = {};

    if(!util::read_bytes(blob_path, widths_blob)) {
        spdlog::warn("bitmap_font: {}: {}", blob_path, util::physfs_error());
        BitmapFont::unload(font);
        return false;
    }

    font.glyph_height = height;
    font.num_pages = widths_blob.size() / UPAGE_AREA;
    font.glyph_widths.resize(widths_blob.size());

    for(std::size_t i = 0; i < widths_blob.size(); ++i) {
        // The Minecraft (pre-1.13) font format requires glyphs to
        // be of uniform height while allowing widths up to the value of height
        font.glyph_widths[i] = (0x0F & (widths_blob[i] >> 4U)) + (0x0F & (widths_blob[i] >> 0U));
        // font.glyph_widths[i] = 0x0F & (widths_blob[i] >> 0U); // cxpr::min<int>(font.glyph_height, widths_blob[i]);
    }

    GLint max_font_pages;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_font_pages);
    font.num_pages = cxpr::min<std::size_t>(font.num_pages, max_font_pages);

    // Currently the height is requested by outside code;
    // FIXME: maybe use the first byte of the glyph sizes blob
    // to determine what height every single glyph in the font is?
    const int tex_size = font.glyph_height * UPAGE_SIZE;

    if(!font.handle)
        glGenTextures(1, &font.handle);
    glBindTexture(GL_TEXTURE_2D_ARRAY, font.handle);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, tex_size, tex_size, font.num_pages, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    for(std::size_t i = 0; i < font.num_pages; ++i) {
        const std::string page_path = fmt::format("fonts/{}.page.{:02X}.png", name, i);

        Image page_image = {};
        if(!Image::load_gray(page_image, page_path, true)) {
            spdlog::warn("bitmap_font: {}: {}", page_path, util::physfs_error());
            continue;
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, page_image.width, page_image.height, 1, GL_RED, GL_UNSIGNED_BYTE, page_image.pixels);
        Image::unload(page_image);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return true;
}

void BitmapFont::unload(BitmapFont &font)
{
    if(font.handle)
        glDeleteTextures(1, &font.handle);
    font.glyph_widths.clear();
    font.glyph_height = 0;
    font.num_pages = 0;
    font.handle = 0;
}
