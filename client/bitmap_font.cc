// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/bitmap_font.hh>
#include <shared/util/physfs.hh>
#include <shared/image.hh>
#include <spdlog/spdlog.hh>

bool BitmapFont::load(BitmapFont &font, int width, int height, const std::string &path)
{
    Image image = {};

    if(Image::load_gray(image, path, true)) {
        GLint max_texture_size = {};
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
        
        if((image.width > max_texture_size) || (image.height > max_texture_size)) {
            spdlog::warn("bitmap_font: {}: image too big ({}x{}) [{}]", path, image.width, image.height, max_texture_size);
            BitmapFont::unload(font);
            Image::unload(image);
            return false;
        }
        
        font.glyph_width = width;
        font.glyph_height = height;
        font.num_glyphs_x = image.width / font.glyph_width;
        font.num_glyphs_y = image.height / font.glyph_height;
        
        if(!font.handle)
            glGenTextures(1, &font.handle);
        glBindTexture(GL_TEXTURE_2D, font.handle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, image.width, image.height, 0, GL_RED, GL_UNSIGNED_BYTE, image.pixels);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        Image::unload(image);
        
        return true;
    }

    spdlog::warn("bitmap_font: {}: {}", path, util::physfs_error());
    BitmapFont::unload(font);
    return false;
}

void BitmapFont::unload(BitmapFont &font)
{
    if(font.handle)
        glDeleteTextures(1, &font.handle);
    font.num_glyphs_y = 0;
    font.num_glyphs_x = 0;
    font.glyph_height = 0;
    font.glyph_width = 0;
    font.handle = 0;
}
