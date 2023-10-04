// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GL_FRAMEBUFFER_HH
#define CLIENT_GL_FRAMEBUFFER_HH
#include <client/gl/renderbuffer.hh>
#include <client/gl/texture.hh>
#include <shared/cxmath.hh>

namespace gl
{
class Framebuffer final : public gl::Object<Framebuffer> {
public:
    Framebuffer() = default;
    Framebuffer(Framebuffer &&rhs);
    Framebuffer &operator=(Framebuffer &&rhs);
    void create();
    void destroy();
    void attach(uint32_t attachment, const gl::Texture2D &texture);
    void attach(uint32_t attachment, const gl::Renderbuffer &rbo);
    bool complete() const;
    void bind() const;
    template<typename... AT>
    void set_fragment_targets(AT &&...args);
    static void unbind();
};
} // namespace gl

inline gl::Framebuffer::Framebuffer(gl::Framebuffer &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline gl::Framebuffer &gl::Framebuffer::operator=(gl::Framebuffer &&rhs)
{
    gl::Framebuffer copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline void gl::Framebuffer::create()
{
    destroy();
    glCreateFramebuffers(1, &handle);
}

inline void gl::Framebuffer::destroy()
{
    if(handle) {
        glDeleteFramebuffers(1, &handle);
        handle = 0;
    }
}

inline void gl::Framebuffer::attach(uint32_t attachment, const gl::Texture2D &texture)
{
    glNamedFramebufferTexture(handle, attachment, texture.get(), 0);
}

inline void gl::Framebuffer::attach(uint32_t attachment, const gl::Renderbuffer &rbo)
{
    glNamedFramebufferRenderbuffer(handle, attachment, GL_RENDERBUFFER, rbo.get());
}

inline bool gl::Framebuffer::complete() const
{
    return glCheckNamedFramebufferStatus(handle, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

inline void gl::Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
}

template<typename... AT>
inline void gl::Framebuffer::set_fragment_targets(AT &&...args)
{
    const uint32_t attachments[] = {static_cast<uint32_t>(args)...};
    glNamedFramebufferDrawBuffers(handle, static_cast<int32_t>(cxmath::array_size(attachments)), attachments);
}

inline void gl::Framebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#endif/* CLIENT_GL_FRAMEBUFFER_HH */
