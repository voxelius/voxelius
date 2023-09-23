// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef F6497C57_54A6_48FC_99DF_2ED45F570563
#define F6497C57_54A6_48FC_99DF_2ED45F570563
#include <client/glxx/object.hh>
#include <client/glxx/pixel_format.hh>

namespace glxx
{
class Renderbuffer final : public Object<Renderbuffer> {
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
    glNamedRenderbufferStorage(handle, glxx::detail::get_pixel_format_gpu(format), width, height);
}

#endif /* F6497C57_54A6_48FC_99DF_2ED45F570563 */
