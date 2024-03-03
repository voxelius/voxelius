// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef CLIENT_GLXX_TEXTURE_HH
#define CLIENT_GLXX_TEXTURE_HH
#include <client/glxx/object.hh>
#include <client/pixel_format.hh>
#include <glm/common.hpp>
#include <shared/cxmath.hh>

namespace glxx
{
class Texture : public glxx::Object<Texture> {
public:
    Texture(uint32_t target);
    void create();
    void destroy();
    void generate_mipmap();
    void bind(uint32_t unit) const;

protected:
    uint32_t target;
};

class Texture2D final : public Texture {
public:
    Texture2D();
    Texture2D(Texture2D &&rhs);
    Texture2D &operator=(Texture2D &&rhs);
    bool storage(int width, int height, PixelFormat format);
    bool write(int x, int y, int width, int height, PixelFormat format, const void *data);
};

class Texture2DArray final : public Texture {
public:
    Texture2DArray();
    Texture2DArray(Texture2DArray &&rhs);
    Texture2DArray &operator=(Texture2DArray &&rhs);
    bool storage(int width, int height, int layers, PixelFormat format);
    bool write(int layer, int x, int y, int width, int height, PixelFormat format, const void *data);
};
} // namespace glxx

inline glxx::Texture::Texture(uint32_t target)
    : target{target}
{
}

inline void glxx::Texture::create()
{
    destroy();
    glCreateTextures(target, 1, &handle);
}

inline void glxx::Texture::destroy()
{
    if(handle) {
        glDeleteTextures(1, &handle);
        handle = 0;
    }
}

inline void glxx::Texture::generate_mipmap()
{
    glGenerateTextureMipmap(handle);
}

inline void glxx::Texture::bind(uint32_t unit) const
{
    glBindTextureUnit(unit, handle);
}

inline glxx::Texture2D::Texture2D()
    : glxx::Texture{GL_TEXTURE_2D}
{
}

inline glxx::Texture2D::Texture2D(glxx::Texture2D &&rhs)
    : glxx::Texture{GL_TEXTURE_2D}
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::Texture2D &glxx::Texture2D::operator=(glxx::Texture2D &&rhs)
{
    glxx::Texture2D copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline bool glxx::Texture2D::storage(int width, int height, PixelFormat format)
{
    if(uint32_t f = detail::get_pixel_format_gpu(format)) {
        glTextureStorage2D(handle, cxmath::log2<int>(cxmath::max(width, height)), f, width, height);
        return true;
    }

    return false;
}

inline bool glxx::Texture2D::write(int x, int y, int width, int height, PixelFormat format, const void *data)
{
    uint32_t fmt, type;
    if(detail::get_pixel_format_cpu(format, fmt, type)) {
        glTextureSubImage2D(handle, 0, x, y, width, height, fmt, type, data);
        return true;
    }

    return false;
}

inline glxx::Texture2DArray::Texture2DArray()
    : glxx::Texture{GL_TEXTURE_2D_ARRAY}
{
}

inline glxx::Texture2DArray::Texture2DArray(glxx::Texture2DArray &&rhs)
    : glxx::Texture{GL_TEXTURE_2D_ARRAY}
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::Texture2DArray &glxx::Texture2DArray::operator=(glxx::Texture2DArray &&rhs)
{
    glxx::Texture2DArray copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline bool glxx::Texture2DArray::storage(int width, int height, int layers, PixelFormat format)
{
    if(uint32_t f = detail::get_pixel_format_gpu(format)) {
        glTextureStorage3D(handle, cxmath::log2<int>(cxmath::max(width, height)), f, width, height, layers);
        return true;
    }

    return false;
}

inline bool glxx::Texture2DArray::write(int layer, int x, int y, int width, int height, PixelFormat format, const void *data)
{
    uint32_t fmt, type;
    if(detail::get_pixel_format_cpu(format, fmt, type)) {
        glTextureSubImage3D(handle, 0, x, y, layer, width, height, 1, fmt, type, data);
        return true;
    }

    return false;
}

#endif /* CLIENT_GLXX_TEXTURE_HH */
