// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef A288DEB5_6CCC_4335_BF93_FEC6FD2E256F
#define A288DEB5_6CCC_4335_BF93_FEC6FD2E256F
#include <client/glxx/object.hh>

namespace glxx
{
class Sampler final : public Object<Sampler> {
public:
    Sampler() = default;
    Sampler(Sampler &&rhs);
    Sampler &operator=(Sampler &&rhs);
    void create();
    void destroy();
    void bind(uint32_t unit) const;
    void parameter(GLenum pname, GLint value);
    void parameter(GLenum pname, GLfloat value);
};
} // namespace glxx

inline glxx::Sampler::Sampler(glxx::Sampler &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::Sampler &glxx::Sampler::operator=(glxx::Sampler &&rhs)
{
    glxx::Sampler copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline void glxx::Sampler::create()
{
    destroy();
    glCreateSamplers(1, &handle);
}

inline void glxx::Sampler::destroy()
{
    if(handle) {
        glDeleteSamplers(1, &handle);
        handle = 0;
    }
}

inline void glxx::Sampler::bind(uint32_t unit) const
{
    glBindSampler(unit, handle);
}

inline void glxx::Sampler::parameter(GLenum pname, GLint value)
{
    glSamplerParameteri(handle, pname, value);
}

inline void glxx::Sampler::parameter(GLenum pname, GLfloat value)
{
    glSamplerParameterf(handle, pname, value);
}

#endif /* A288DEB5_6CCC_4335_BF93_FEC6FD2E256F */
