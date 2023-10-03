// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GL_PIXEL_FORMAT_HH
#define CLIENT_GL_PIXEL_FORMAT_HH
#include <glad/gl.h>

enum class PixelFormat {
    R8_UNORM,
    R8_SINT,
    R8_UINT,
    R8G8_UNORM,
    R8G8_SINT,
    R8G8_UINT,
    R8G8B8_UNORM,
    R8G8B8_SINT,
    R8G8B8_UINT,
    R8G8B8A8_UNORM,
    R8G8B8A8_SINT,
    R8G8B8A8_UINT,
    R16_UNORM,
    R16_SINT,
    R16_UINT,
    R16_FLOAT,
    R16G16_UNORM,
    R16G16_SINT,
    R16G16_UINT,
    R16G16_FLOAT,
    R16G16B16_UNORM,
    R16G16B16_SINT,
    R16G16B16_UINT,
    R16G16B16_FLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_SINT,
    R16G16B16A16_UINT,
    R16G16B16A16_FLOAT,
    R32_SINT,
    R32_UINT,
    R32_FLOAT,
    R32G32_SINT,
    R32G32_UINT,
    R32G32_FLOAT,
    R32G32B32_SINT,
    R32G32B32_UINT,
    R32G32B32_FLOAT,
    R32G32B32A32_SINT,
    R32G32B32A32_UINT,
    R32G32B32A32_FLOAT,
    D16_UNORM,
    D24_UNORM,
    D24_UNORM_S8_UINT,
    D32_UNORM,
    D32_FLOAT,
    D32_FLOAT_S8_UINT,
    S8_UINT,
};

namespace detail
{
static inline bool get_pixel_format_cpu(PixelFormat format, uint32_t &fmt, uint32_t &type)
{
    switch(format) {
        case PixelFormat::R8_UNORM:
        case PixelFormat::R16_UNORM:
        case PixelFormat::R16_FLOAT:
        case PixelFormat::R32_FLOAT:
            fmt = GL_RED;
            break;
        case PixelFormat::R8_SINT:
        case PixelFormat::R8_UINT:
        case PixelFormat::R16_SINT:
        case PixelFormat::R16_UINT:
        case PixelFormat::R32_SINT:
        case PixelFormat::R32_UINT:
            fmt = GL_RED_INTEGER;
            break;
        case PixelFormat::R8G8_UNORM:
        case PixelFormat::R16G16_UNORM:
        case PixelFormat::R16G16_FLOAT:
        case PixelFormat::R32G32_FLOAT:
            fmt = GL_RG;
            break;
        case PixelFormat::R8G8_SINT:
        case PixelFormat::R8G8_UINT:
        case PixelFormat::R16G16_SINT:
        case PixelFormat::R16G16_UINT:
        case PixelFormat::R32G32_SINT:
        case PixelFormat::R32G32_UINT:
            fmt = GL_RG_INTEGER;
            break;
        case PixelFormat::R8G8B8_UNORM:
        case PixelFormat::R16G16B16_UNORM:
        case PixelFormat::R16G16B16_FLOAT:
        case PixelFormat::R32G32B32_FLOAT:
            fmt = GL_RGB;
            break;
        case PixelFormat::R8G8B8_SINT:
        case PixelFormat::R8G8B8_UINT:
        case PixelFormat::R16G16B16_SINT:
        case PixelFormat::R16G16B16_UINT:
        case PixelFormat::R32G32B32_SINT:
        case PixelFormat::R32G32B32_UINT:
            fmt = GL_RGB_INTEGER;
            break;
        case PixelFormat::R8G8B8A8_UNORM:
        case PixelFormat::R16G16B16A16_UNORM:
        case PixelFormat::R16G16B16A16_FLOAT:
        case PixelFormat::R32G32B32A32_FLOAT:
            fmt = GL_RGBA;
            break;
        case PixelFormat::R8G8B8A8_SINT:
        case PixelFormat::R8G8B8A8_UINT:
        case PixelFormat::R16G16B16A16_SINT:
        case PixelFormat::R16G16B16A16_UINT:
        case PixelFormat::R32G32B32A32_SINT:
        case PixelFormat::R32G32B32A32_UINT:
            fmt = GL_RGBA_INTEGER;
            break;
        default:
            return false;
    }

    switch(format) {
        case PixelFormat::R8_SINT:
        case PixelFormat::R8G8_SINT:
        case PixelFormat::R8G8B8_SINT:
        case PixelFormat::R8G8B8A8_SINT:
            type = GL_BYTE;
            break;
        case PixelFormat::R8_UNORM:
        case PixelFormat::R8_UINT:
        case PixelFormat::R8G8_UNORM:
        case PixelFormat::R8G8_UINT:
        case PixelFormat::R8G8B8_UNORM:
        case PixelFormat::R8G8B8_UINT:
        case PixelFormat::R8G8B8A8_UNORM:
        case PixelFormat::R8G8B8A8_UINT:
            type = GL_UNSIGNED_BYTE;
            break;
        case PixelFormat::R16_SINT:
        case PixelFormat::R16G16_SINT:
        case PixelFormat::R16G16B16_SINT:
        case PixelFormat::R16G16B16A16_SINT:
            type = GL_SHORT;
            break;
        case PixelFormat::R16_UNORM:
        case PixelFormat::R16_UINT:
        case PixelFormat::R16G16_UNORM:
        case PixelFormat::R16G16_UINT:
        case PixelFormat::R16G16B16_UNORM:
        case PixelFormat::R16G16B16_UINT:
        case PixelFormat::R16G16B16A16_UNORM:
        case PixelFormat::R16G16B16A16_UINT:
            type = GL_UNSIGNED_SHORT;
            break;
        case PixelFormat::R32_SINT:
        case PixelFormat::R32G32_SINT:
        case PixelFormat::R32G32B32_SINT:
        case PixelFormat::R32G32B32A32_SINT:
            type = GL_INT;
            break;
        case PixelFormat::R32_UINT:
        case PixelFormat::R32G32_UINT:
        case PixelFormat::R32G32B32_UINT:
        case PixelFormat::R32G32B32A32_UINT:
            type = GL_UNSIGNED_INT;
            break;
        case PixelFormat::R32_FLOAT:
        case PixelFormat::R32G32_FLOAT:
        case PixelFormat::R32G32B32_FLOAT:
        case PixelFormat::R32G32B32A32_FLOAT:
            type = GL_FLOAT;
            break;
        default:
            return false;
    }

    return true;
}

static inline uint32_t get_pixel_format_gpu(PixelFormat format)
{
    switch(format) {
        case PixelFormat::R8_UNORM:
            return GL_R8;
        case PixelFormat::R8_SINT:
            return GL_R8I;
        case PixelFormat::R8_UINT:
            return GL_R8UI;
        case PixelFormat::R8G8_UNORM:
            return GL_RG8;
        case PixelFormat::R8G8_SINT:
            return GL_RG8I;
        case PixelFormat::R8G8_UINT:
            return GL_RG8UI;
        case PixelFormat::R8G8B8_UNORM:
            return GL_RGB8;
        case PixelFormat::R8G8B8_SINT:
            return GL_RGB8I;
        case PixelFormat::R8G8B8_UINT:
            return GL_RGB8UI;
        case PixelFormat::R8G8B8A8_UNORM:
            return GL_RGBA8;
        case PixelFormat::R8G8B8A8_SINT:
            return GL_RGBA8I;
        case PixelFormat::R8G8B8A8_UINT:
            return GL_RGBA8UI;
        case PixelFormat::R16_UNORM:
            return GL_R16;
        case PixelFormat::R16_SINT:
            return GL_R16I;
        case PixelFormat::R16_UINT:
            return GL_R16UI;
        case PixelFormat::R16_FLOAT:
            return GL_R16F;
        case PixelFormat::R16G16_UNORM:
            return GL_RG16;
        case PixelFormat::R16G16_SINT:
            return GL_RG16I;
        case PixelFormat::R16G16_UINT:
            return GL_RG16UI;
        case PixelFormat::R16G16_FLOAT:
            return GL_RG16F;
        case PixelFormat::R16G16B16_UNORM:
            return GL_RGB16;
        case PixelFormat::R16G16B16_SINT:
            return GL_RGB16I;
        case PixelFormat::R16G16B16_UINT:
            return GL_RGB16UI;
        case PixelFormat::R16G16B16_FLOAT:
            return GL_RGB16F;
        case PixelFormat::R16G16B16A16_UNORM:
            return GL_RGBA16;
        case PixelFormat::R16G16B16A16_SINT:
            return GL_RGBA16I;
        case PixelFormat::R16G16B16A16_UINT:
            return GL_RGBA16UI;
        case PixelFormat::R16G16B16A16_FLOAT:
            return GL_RGBA16F;
        case PixelFormat::R32_SINT:
            return GL_R32I;
        case PixelFormat::R32_UINT:
            return GL_R32UI;
        case PixelFormat::R32_FLOAT:
            return GL_R32F;
        case PixelFormat::R32G32_SINT:
            return GL_RG32I;
        case PixelFormat::R32G32_UINT:
            return GL_RG32UI;
        case PixelFormat::R32G32_FLOAT:
            return GL_RG32F;
        case PixelFormat::R32G32B32_SINT:
            return GL_RGB32I;
        case PixelFormat::R32G32B32_UINT:
            return GL_RGB32UI;
        case PixelFormat::R32G32B32_FLOAT:
            return GL_RGB32F;
        case PixelFormat::R32G32B32A32_SINT:
            return GL_RGBA32I;
        case PixelFormat::R32G32B32A32_UINT:
            return GL_RGBA32UI;
        case PixelFormat::R32G32B32A32_FLOAT:
            return GL_RGBA32F;
        case PixelFormat::D16_UNORM:
            return GL_DEPTH_COMPONENT16;
        case PixelFormat::D24_UNORM:
            return GL_DEPTH_COMPONENT24;
        case PixelFormat::D24_UNORM_S8_UINT:
            return GL_DEPTH24_STENCIL8;
        case PixelFormat::D32_UNORM:
            return GL_DEPTH_COMPONENT32;
        case PixelFormat::D32_FLOAT:
            return GL_DEPTH_COMPONENT32F;
        case PixelFormat::D32_FLOAT_S8_UINT:
            return GL_DEPTH32F_STENCIL8;
        case PixelFormat::S8_UINT:
            return GL_STENCIL_INDEX8;
        default:
            return 0;
    }
}
} // namespace detail

#endif/* CLIENT_GL_PIXEL_FORMAT_HH */
