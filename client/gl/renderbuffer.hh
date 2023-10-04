// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GL_RENDERBUFFER_HH
#define CLIENT_GL_RENDERBUFFER_HH
#include <client/gl/object.hh>
#include <client/pixel_format.hh>

namespace gl
{
class Renderbuffer final : public gl::Object<Renderbuffer> {
public:
    Renderbuffer() = default;
    Renderbuffer(Renderbuffer &&rhs);
    Renderbuffer &operator=(Renderbuffer &&rhs);
    void create();
    void destroy();
    void storage(int width, int height, PixelFormat format);
};
} // namespace gl

inline gl::Renderbuffer::Renderbuffer(gl::Renderbuffer &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline gl::Renderbuffer &gl::Renderbuffer::operator=(gl::Renderbuffer &&rhs)
{
    gl::Renderbuffer copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline void gl::Renderbuffer::create()
{
    destroy();
    glCreateRenderbuffers(1, &handle);
}

inline void gl::Renderbuffer::destroy()
{
    if(handle) {
        glDeleteRenderbuffers(1, &handle);
        handle = 0;
    }
}

inline void gl::Renderbuffer::storage(int width, int height, PixelFormat format)
{
    glNamedRenderbufferStorage(handle, detail::get_pixel_format_gpu(format), width, height);
}

#endif/* CLIENT_GL_RENDERBUFFER_HH */
