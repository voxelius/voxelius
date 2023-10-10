// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLXX_SAMPLER_HH
#define CLIENT_GLXX_SAMPLER_HH
#include <client/glxx/object.hh>

namespace glxx
{
class Sampler final : public glxx::Object<Sampler> {
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

inline void glxx::Sampler::parameter(uint32_t pname, int32_t value)
{
    glSamplerParameteri(handle, pname, value);
}

inline void glxx::Sampler::parameter(uint32_t pname, GLfloat value)
{
    glSamplerParameterf(handle, pname, value);
}

#endif /* CLIENT_GLXX_SAMPLER_HH */
