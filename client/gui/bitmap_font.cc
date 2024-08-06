// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui/bitmap_font.hh>
#include <shared/math/constexpr.hh>
#include <shared/util/physfs.hh>
#include <parson.h>
#include <shared/image.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

bool BitmapFont::load(BitmapFont &font, const std::string &path)
{
    std::string source = {};

    if(!util::read_string(path, source)) {
        spdlog::warn("bitmap_font: {}: {}", path, util::physfs_error());
        BitmapFont::unload(font);
        return false;
    }

    JSON_Value *jsonv = json_parse_string(source.c_str());
    const JSON_Object *json = json_value_get_object(jsonv);

    if(!jsonv) {
        spdlog::warn("bitmap_font: {}: parse error", path);
        BitmapFont::unload(font);
        return false;
    }

    if(!json) {
        spdlog::warn("bitmap_font: {}: root is not an object", path);
        BitmapFont::unload(font);
        json_value_free(jsonv);
        return false;
    }

    // If not specified, use Minecraft's default of 16
    font.glyph_height = static_cast<int>(json_object_get_number(json, "glyph_height"));
    font.glyph_height = font.glyph_height ? font.glyph_height : 16;

    // If not specified, use Minecraft's default path
    // NOTE: this would mean all fonts will pull the same file
    const char *blob_path_cstr = json_object_get_string(json, "glyph_sizes_path");
    const std::string blob_path = blob_path_cstr ? blob_path_cstr : std::string("fonts/glyph_sizes.bin");
    std::vector<std::uint8_t> blob = {};

    if(!util::read_bytes(blob_path, blob)) {
        spdlog::warn("bitmap_font: {}: {}", blob_path, util::physfs_error());
        BitmapFont::unload(font);
        json_value_free(jsonv);
        return false;
    }

    GLint max_font_pages;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_font_pages);
    font.num_pages = cxpr::min<std::size_t>(blob.size() / PAGE_SIZE / PAGE_SIZE, max_font_pages);

    font.glyph_offsets.resize(font.num_pages);
    font.glyph_widths.resize(font.num_pages);

    for(std::size_t i = 0; i < font.num_pages; ++i) {
        font.glyph_offsets[i] = (0x0F & (blob[i] >> 4U));
        font.glyph_widths[i] = (0x0F & (blob[i] >> 0U)) + 2;
    }

    // If not specified, use Minecraft's default path
    // NOTE: the main difference with Minecraft here is that
    // in Voxelius all the font assets are kept in a single subdirectory
    const char *page_fmt_cstr = json_object_get_string(json, "unicode_page_fmt");
    const std::string page_fmt = page_fmt_cstr ? page_fmt_cstr : std::string("fonts/unicode_page_{:02x}.png");

    // This is constant and must not change ever
    const int tex_size = font.glyph_height * PAGE_SIZE;

    if(!font.handle)
        glGenTextures(1, &font.handle);
    glBindTexture(GL_TEXTURE_2D_ARRAY, font.handle);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, tex_size, tex_size, font.num_pages, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    for(std::size_t i = 0; i < font.num_pages; ++i) {
        const std::string page_path = fmt::format(page_fmt, i);

        Image page_image = {};
        if(!Image::load_gray(page_image, page_path, true)) {
            spdlog::warn("bitmap_font: {}: {}", page_path, util::physfs_error());
            continue;
        }

        if((page_image.width > tex_size) || (page_image.height > tex_size)) {
            spdlog::warn("bitmap_font: {}: page image size mismatch: {}x{} vs [{}x{}]", page_image.width, page_image.height, tex_size, tex_size);
            Image::unload(page_image);
            continue;
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, page_image.width, page_image.height, 1, GL_RED, GL_UNSIGNED_BYTE, page_image.pixels);
        Image::unload(page_image);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    json_value_free(jsonv);

    return true;
}

void BitmapFont::unload(BitmapFont &font)
{
    if(font.handle)
        glDeleteTextures(1, &font.handle);
    font.glyph_offsets.clear();
    font.glyph_widths.clear();
    font.glyph_height = 0;
    font.num_pages = 0;
    font.handle = 0;
}
