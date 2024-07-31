// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/canvas/font.hh>
#include <shared/util/physfs.hh>
#include <shared/image.hh>
#include <spdlog/spdlog.h>

bool Font::load(Font &font, int glyph_width, int glyph_height, const std::string &path)
{
    Image image = {};

    if(Image::load_gray(image, path, true)) {
        GLint max_texture_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

        if((image.width > max_texture_size) || (image.height > max_texture_size)) {
            spdlog::warn("font: {}: texture too big ({}x{}) [{}]", path, image.width, image.height, max_texture_size);
            Image::unload(image);
            Font::unload(font);
            return false;
        }

        font.glyph_width = glyph_width;
        font.glyph_height = glyph_height;
        font.texture_cwidth = image.width / glyph_width;
        font.texture_cheight = image.height / glyph_height;

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

    spdlog::warn("font: {}: {}", path, util::physfs_error());
    Font::unload(font);
    return false;
}

void Font::unload(Font &font)
{
    if(font.handle)
        glDeleteTextures(1, &font.handle);
    font.texture_cheight = 0;
    font.texture_cwidth = 0;
    font.glyph_height = 0;
    font.glyph_width = 0;
    font.handle = 0;
}
