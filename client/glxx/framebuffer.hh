// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLXX_FRAMEBUFFER_HH
#define CLIENT_GLXX_FRAMEBUFFER_HH
#include <client/glxx/renderbuffer.hh>
#include <client/glxx/texture.hh>
#include <shared/cxmath.hh>

namespace glxx
{
class Framebuffer final : public Object<Framebuffer> {
public:
    Framebuffer() = default;
    Framebuffer(Framebuffer &&rhs);
    Framebuffer &operator=(Framebuffer &&rhs);
    void create();
    void destroy();
    void attach(uint32_t attachment, const Texture2D &texture);
    void attach(uint32_t attachment, const Renderbuffer &rbo);
    bool complete() const;
    void bind() const;
    template<typename... AT>
    void set_fragment_targets(AT &&...args);
    static void unbind();
};
} // namespace glxx

inline glxx::Framebuffer::Framebuffer(glxx::Framebuffer &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::Framebuffer &glxx::Framebuffer::operator=(glxx::Framebuffer &&rhs)
{
    glxx::Framebuffer copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline void glxx::Framebuffer::create()
{
    destroy();
    glCreateFramebuffers(1, &handle);
}

inline void glxx::Framebuffer::destroy()
{
    if(handle) {
        glDeleteFramebuffers(1, &handle);
        handle = 0;
    }
}

inline void glxx::Framebuffer::attach(uint32_t attachment, const glxx::Texture2D &texture)
{
    glNamedFramebufferTexture(handle, attachment, texture.get(), 0);
}

inline void glxx::Framebuffer::attach(uint32_t attachment, const glxx::Renderbuffer &rbo)
{
    glNamedFramebufferRenderbuffer(handle, attachment, GL_RENDERBUFFER, rbo.get());
}

inline bool glxx::Framebuffer::complete() const
{
    return glCheckNamedFramebufferStatus(handle, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

inline void glxx::Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
}

template<typename... AT>
inline void glxx::Framebuffer::set_fragment_targets(AT &&...args)
{
    const uint32_t attachments[] = {static_cast<uint32_t>(args)...};
    glNamedFramebufferDrawBuffers(handle, static_cast<int32_t>(cxmath::array_size(attachments)), attachments);
}

inline void glxx::Framebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#endif /* CLIENT_GLXX_FRAMEBUFFER_HH */
