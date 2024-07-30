// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/canvas/font.hh>
#include <shared/util/physfs.hh>
#include <shared/image.hh>
#include <spdlog/spdlog.h>

bool Font::load_binary(Font &font, int width, int height, const std::string &path)
{
    Font::unload(font);

    return false; // UNDONE
}

bool Font::load_image(Font &font, int width, int height, const std::string &path)
{
    Image image = {};

    Font::unload(font);

    if(Image::load_gray(image, path, true)) {
        font.glyph_width = width;
        font.glyph_height = height;
        font.texture_x_glyphs = image.width / width;
        font.texture_y_glyphs = image.height / height;

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

    spdlog::warn("Font: {}: {}", path, util::physfs_error());

    return false;
}

void Font::unload(Font &font)
{
    if(font.handle)
        glDeleteTextures(1, &font.handle);
    font.texture_y_glyphs = 0;
    font.texture_x_glyphs = 0;
    font.glyph_height = 0;
    font.glyph_width = 0;
    font.handle = 0U;
}
