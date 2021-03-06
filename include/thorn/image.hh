/*
 * image.hh
 * Created: 2021-03-10, 08:53:03.
 * Copyright (C) 2021, Kirill GPRB.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <glad/gl.h>
#include <stb_image.h>
#include <stdint.h>
#include <vector>

namespace thorn
{
/**
 * @brief RGBA image container.
 * 
 */
class Image {
public:
    /**
     * @brief Constructor.
     * 
     */
    Image();

    /**
     * @brief Move constructor.
     * 
     * @param rhs Existing image.
     */
    Image(Image &&rhs);
    Image(const Image &rhs) = delete;

    /**
     * @brief Assign operator.
     * 
     * @param rhs Existing image
     * @return this
     */
    Image &operator=(Image &&rhs);
    Image &operator=(const Image &rhs) = delete;

    /**
     * @brief Destructor.
     * 
     */
    virtual ~Image();

    /**
     * @brief Destructor
     * 
     */
    void clear();

    /**
     * @brief Creates a pixel buffer.
     * 
     * @param binary Compresed image data.
     * @return true if succeeded, false otherwise.
     */
    bool load(const std::vector<uint8_t> &binary);

    /**
     * @brief Gets the width of the Image.
     * 
     * @return Width in pixels.
     */
    constexpr int getWidth() const;

    /**
     * @brief Gets the height of the image.
     * 
     * @return Height in pixels.
     */
    constexpr int getHeight() const;

    /**
     * @brief Gets the size of the image.
     * 
     * @param width Width in pixels.
     * @param height Height in pixels.
     */
    constexpr void getSize(int &width, int &height) const;

    /**
     * @brief Gets the pointer to the pixel buffer.
     * 
     * @return Pixel buffer pointer.
     */
    constexpr const void *getPixels() const;

    /**
     * @brief Gets the pointer to the pixel buffer.
     * 
     * @return Pixel buffer pointer.
     */
    constexpr void *getPixels();

public:
    static const GLenum TEXTURE_FORMAT = GL_RGBA;
    static const GLenum TEXTURE_TYPE = GL_UNSIGNED_BYTE;

private:
    int width, height;
    unsigned char *pixels;
};

inline Image::Image()
{
    width = -1;
    height = -1;
    pixels = nullptr;
}

inline Image::Image(Image &&rhs)
{
    width = rhs.width;
    height = rhs.height;
    pixels = rhs.pixels;
    rhs.width = -1;
    rhs.height = -1;
    rhs.pixels = nullptr;
}

inline Image::~Image()
{
    stbi_image_free(pixels);
}

inline Image &Image::operator=(Image &&rhs)
{
    Image copy(std::move(rhs));
    std::swap(copy.width, width);
    std::swap(copy.height, height);
    std::swap(copy.pixels, pixels);
    return *this;
}

inline void Image::clear()
{
    stbi_image_free(pixels);
    width = -1;
    height = -1;
    pixels = nullptr;
}

inline bool Image::load(const std::vector<uint8_t> &binary)
{
    stbi_image_free(pixels);
    pixels = stbi_load_from_memory(binary.data(), static_cast<int>(binary.size()), &width, &height, nullptr, STBI_rgb_alpha);
    return !!pixels;
}

inline constexpr int Image::getWidth() const
{
    return width;
}

inline constexpr int Image::getHeight() const
{
    return height;
}

inline constexpr void Image::getSize(int &width, int &height) const
{
    width = this->width;
    height = this->height;
}

inline constexpr const void *Image::getPixels() const
{
    return pixels;
}

inline constexpr void *Image::getPixels()
{
    return pixels;
}
} // namespace thorn
