// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/font.hh>
#include <schrift.h>
#include <shared/math/constexpr.hh>
#include <shared/util/physfs.hh>
#include <spdlog/spdlog.h>

bool Font::load(Font &font, char32_t maxcp, int height, const std::string &path)
{
    std::vector<std::uint8_t> buffer = {};

    if(!util::read_bytes(path, buffer)) {
        spdlog::warn("font: {}: {}", path, util::physfs_error());
        Font::unload(font);
        return false;
    }

    SFT sft = {};
    sft.font = sft_loadmem(buffer.data(), buffer.size());
    sft.xScale = static_cast<double>(height);
    sft.yScale = static_cast<double>(height);
    sft.yOffset = 2.0;

    if(!sft.font) {
        spdlog::warn("font: {}: schrift parse error", path);
        Font::unload(font);
        return false;
    }

    const int tex_size = height * PAGE_SIZE;
    const char32_t num_codepoints = maxcp + 1U;

    GLint max_font_pages;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_font_pages);
    font.num_pages = cxpr::min<std::size_t>(num_codepoints / PAGE_SIZE / PAGE_SIZE, max_font_pages);
    font.gmetrics.resize(num_codepoints);
    font.glyph_height = height;

    if(!font.handle)
        glGenTextures(1, &font.handle);
    glBindTexture(GL_TEXTURE_2D_ARRAY, font.handle);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, tex_size, tex_size, font.num_pages, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    SFT_Glyph glyph = {};
    SFT_Image image = {};
    image.width = font.glyph_height;
    image.height = font.glyph_height;
    image.pixels = new std::uint8_t[font.glyph_height * font.glyph_height];

    char32_t unicode = 0;
    for(std::size_t page = 0; page < font.num_pages; ++page) {
        for(int ypos = 0; ypos < tex_size; ypos += font.glyph_height)
        for(int xpos = 0; xpos < tex_size; xpos += font.glyph_height) {
            sft_lookup(&sft, static_cast<SFT_UChar>(unicode), &glyph);
            sft_gmetrics(&sft, glyph, &font.gmetrics[unicode]);
            sft_render(&sft, glyph, image);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, xpos, tex_size - ypos - font.glyph_height, page, font.glyph_height, font.glyph_height, 1, GL_RED, GL_UNSIGNED_BYTE, image.pixels);
            font.gmetrics[unicode].advanceWidth += 1.0;
            unicode += 1U;
        }
    }

    sft_lmetrics(&sft, &font.lmetrics);

    // SFT_Image::pixels is a typeless pointer
    delete reinterpret_cast<std::uint8_t *>(image.pixels);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    sft_freefont(sft.font);

    return true;
}

void Font::unload(Font &font)
{
    if(font.handle)
        glDeleteTextures(1, &font.handle);
    font.gmetrics.clear();
    font.glyph_height = 0;
    font.num_pages = 0;
    font.handle = 0;
}
