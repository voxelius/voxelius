// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef E7A03B69_CB3F_461D_9AEE_494C746DEEE4
#define E7A03B69_CB3F_461D_9AEE_494C746DEEE4
#include <client/glxx/object.hh>
#include <client/glxx/pixel_format.hh>
#include <shared/cxmath.hh>
#include <glm/common.hpp>

namespace glxx
{
class Texture : public Object<Texture> {
public:
    Texture(GLenum target);
    void create();
    void destroy();
    void gen_mipmap();
    void bind(uint32_t unit) const;

protected:
    GLenum target;
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

inline glxx::Texture::Texture(GLenum target)
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

inline void glxx::Texture::gen_mipmap()
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

inline bool glxx::Texture2D::storage(int width, int height, glxx::PixelFormat format)
{
    if(GLenum f = glxx::detail::get_pixel_format_gpu(format)) {
        glTextureStorage2D(handle, cxmath::log2<int>(cxmath::max(width, height)), f, width, height);
        return true;
    }

    return false;
}

inline bool glxx::Texture2D::write(int x, int y, int width, int height, glxx::PixelFormat format, const void *data)
{
    GLenum fmt, type;
    if(glxx::detail::get_pixel_format_cpu(format, fmt, type)) {
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

inline bool glxx::Texture2DArray::storage(int width, int height, int layers, glxx::PixelFormat format)
{
    if(GLenum f = glxx::detail::get_pixel_format_gpu(format)) {
        glTextureStorage3D(handle, cxmath::log2<int>(cxmath::max(width, height)), f, width, height, layers);
        return true;
    }

    return false;
}

inline bool glxx::Texture2DArray::write(int layer, int x, int y, int width, int height, PixelFormat format, const void *data)
{
    GLenum fmt, type;
    if(glxx::detail::get_pixel_format_cpu(format, fmt, type)) {
        glTextureSubImage3D(handle, 0, x, y, layer, width, height, 1, fmt, type, data);
        return true;
    }

    return false;
}

#endif /* E7A03B69_CB3F_461D_9AEE_494C746DEEE4 */
