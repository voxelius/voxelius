// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef CLIENT_GLXX_RENDERBUFFER_HH
#define CLIENT_GLXX_RENDERBUFFER_HH
#include <client/glxx/object.hh>
#include <client/pixel_format.hh>

namespace glxx
{
class Renderbuffer final : public glxx::Object<Renderbuffer> {
public:
    Renderbuffer() = default;
    Renderbuffer(Renderbuffer &&rhs);
    Renderbuffer &operator=(Renderbuffer &&rhs);
    void create();
    void destroy();
    void storage(int width, int height, PixelFormat format);
};
} // namespace glxx

inline glxx::Renderbuffer::Renderbuffer(glxx::Renderbuffer &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::Renderbuffer &glxx::Renderbuffer::operator=(glxx::Renderbuffer &&rhs)
{
    glxx::Renderbuffer copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline void glxx::Renderbuffer::create()
{
    destroy();
    glCreateRenderbuffers(1, &handle);
}

inline void glxx::Renderbuffer::destroy()
{
    if(handle) {
        glDeleteRenderbuffers(1, &handle);
        handle = 0;
    }
}

inline void glxx::Renderbuffer::storage(int width, int height, PixelFormat format)
{
    glNamedRenderbufferStorage(handle, detail::get_pixel_format_gpu(format), width, height);
}

#endif /* CLIENT_GLXX_RENDERBUFFER_HH */
