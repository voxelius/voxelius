// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GL_TEXTURE_HH
#define CLIENT_GL_TEXTURE_HH
#include <client/gl/object.hh>
#include <client/pixel_format.hh>
#include <shared/cxmath.hh>
#include <glm/common.hpp>

namespace gl
{
class Texture : public gl::Object<Texture> {
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
} // namespace gl

inline gl::Texture::Texture(uint32_t target)
    : target{target}
{
}

inline void gl::Texture::create()
{
    destroy();
    glCreateTextures(target, 1, &handle);
}

inline void gl::Texture::destroy()
{
    if(handle) {
        glDeleteTextures(1, &handle);
        handle = 0;
    }
}

inline void gl::Texture::generate_mipmap()
{
    glGenerateTextureMipmap(handle);
}

inline void gl::Texture::bind(uint32_t unit) const
{
    glBindTextureUnit(unit, handle);
}

inline gl::Texture2D::Texture2D()
    : gl::Texture{GL_TEXTURE_2D}
{
}

inline gl::Texture2D::Texture2D(gl::Texture2D &&rhs)
    : gl::Texture{GL_TEXTURE_2D}
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline gl::Texture2D &gl::Texture2D::operator=(gl::Texture2D &&rhs)
{
    gl::Texture2D copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline bool gl::Texture2D::storage(int width, int height, PixelFormat format)
{
    if(uint32_t f = detail::get_pixel_format_gpu(format)) {
        glTextureStorage2D(handle, cxmath::log2<int>(cxmath::max(width, height)), f, width, height);
        return true;
    }

    return false;
}

inline bool gl::Texture2D::write(int x, int y, int width, int height, PixelFormat format, const void *data)
{
    uint32_t fmt, type;
    if(detail::get_pixel_format_cpu(format, fmt, type)) {
        glTextureSubImage2D(handle, 0, x, y, width, height, fmt, type, data);
        return true;
    }

    return false;
}

inline gl::Texture2DArray::Texture2DArray()
    : gl::Texture{GL_TEXTURE_2D_ARRAY}
{
}

inline gl::Texture2DArray::Texture2DArray(gl::Texture2DArray &&rhs)
    : gl::Texture{GL_TEXTURE_2D_ARRAY}
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline gl::Texture2DArray &gl::Texture2DArray::operator=(gl::Texture2DArray &&rhs)
{
    gl::Texture2DArray copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline bool gl::Texture2DArray::storage(int width, int height, int layers, PixelFormat format)
{
    if(uint32_t f = detail::get_pixel_format_gpu(format)) {
        glTextureStorage3D(handle, cxmath::log2<int>(cxmath::max(width, height)), f, width, height, layers);
        return true;
    }

    return false;
}

inline bool gl::Texture2DArray::write(int layer, int x, int y, int width, int height, PixelFormat format, const void *data)
{
    uint32_t fmt, type;
    if(detail::get_pixel_format_cpu(format, fmt, type)) {
        glTextureSubImage3D(handle, 0, x, y, layer, width, height, 1, fmt, type, data);
        return true;
    }

    return false;
}

#endif/* CLIENT_GL_TEXTURE_HH */
