// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GL_SAMPLER_HH
#define CLIENT_GL_SAMPLER_HH
#include <client/gl_object.hh>

namespace gl
{
class Sampler final : public gl::Object<Sampler> {
public:
    Sampler() = default;
    Sampler(Sampler &&rhs);
    Sampler &operator=(Sampler &&rhs);
    void create();
    void destroy();
    void bind(uint32_t unit) const;
    void parameter(uint32_t pname, int32_t value);
    void parameter(uint32_t pname, GLfloat value);
};
} // namespace gl

inline gl::Sampler::Sampler(gl::Sampler &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline gl::Sampler &gl::Sampler::operator=(gl::Sampler &&rhs)
{
    gl::Sampler copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline void gl::Sampler::create()
{
    destroy();
    glCreateSamplers(1, &handle);
}

inline void gl::Sampler::destroy()
{
    if(handle) {
        glDeleteSamplers(1, &handle);
        handle = 0;
    }
}

inline void gl::Sampler::bind(uint32_t unit) const
{
    glBindSampler(unit, handle);
}

inline void gl::Sampler::parameter(uint32_t pname, int32_t value)
{
    glSamplerParameteri(handle, pname, value);
}

inline void gl::Sampler::parameter(uint32_t pname, GLfloat value)
{
    glSamplerParameterf(handle, pname, value);
}

#endif /* CLIENT_GL_SAMPLER_HH */
