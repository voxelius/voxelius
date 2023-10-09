// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_RENDER_GL_PROGRAM_HH
#define CLIENT_RENDER_GL_PROGRAM_HH
#include <array>
#include <client/render/gl/shader.hh>
#include <spdlog/spdlog.h>

namespace gl
{
class Program final : public gl::Object<Program> {
public:
    Program() = default;
    Program(Program &&rhs);
    Program &operator=(Program &&rhs);
    void create();
    void destroy();
    void set_separable(bool separable);
    void attach(const gl::Shader &shader);
    bool link();
    void bind();
    constexpr bool is_separable() const;
    constexpr GLbitfield get_stage_bits() const;
    static void unbind();

private:
    bool separable {false};
    GLbitfield stage_bits {0};
};
} // namespace gl

namespace detail
{
static inline GLbitfield get_stage_bit(uint32_t stage)
{
    if(stage == GL_VERTEX_SHADER)
        return GL_VERTEX_SHADER_BIT;
    if(stage == GL_FRAGMENT_SHADER)
        return GL_FRAGMENT_SHADER_BIT;
    return 0;
}

static inline void check_program_info_log(uint32_t program)
{
    int32_t length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if(length > 1) {
        std::string info_log(static_cast<size_t>(length) + 1, static_cast<char>(0));
        glGetProgramInfoLog(program, static_cast<int32_t>(info_log.size()), nullptr, info_log.data());
        spdlog::debug(info_log);
    }
}
} // namespace detail

inline gl::Program::Program(gl::Program &&rhs)
    : separable{rhs.separable}, stage_bits{rhs.stage_bits}
{
    handle = rhs.handle;
    rhs.handle = 0;
    rhs.separable = false;
    rhs.stage_bits = 0;
}

inline gl::Program &gl::Program::operator=(gl::Program &&rhs)
{
    gl::Program copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    std::swap(separable, copy.separable);
    std::swap(stage_bits, copy.stage_bits);
    return *this;
}

inline void gl::Program::create()
{
    destroy();
    handle = glCreateProgram();
    glProgramParameteri(handle, GL_PROGRAM_SEPARABLE, separable ? GL_TRUE : GL_FALSE);
}

inline void gl::Program::destroy()
{
    if(handle) {
        glDeleteProgram(handle);
        stage_bits = 0;
        handle = 0;
    }
}

inline void gl::Program::set_separable(bool separable)
{
    if(handle)
        glProgramParameteri(handle, GL_PROGRAM_SEPARABLE, separable ? GL_TRUE : GL_FALSE);
    this->separable = separable;
}

inline void gl::Program::attach(const gl::Shader &shader)
{
    const GLbitfield stage_bit = detail::get_stage_bit(shader.get_stage());
    if(handle && !(stage_bits & stage_bit)) {
        glAttachShader(handle, shader.get());
        stage_bits |= stage_bit;
    }
}

inline bool gl::Program::link()
{
    if(handle && stage_bits) {
        int32_t status;

        glLinkProgram(handle);
        detail::check_program_info_log(handle);

        glGetProgramiv(handle, GL_LINK_STATUS, &status);
        return status;
    }

    return false;
}

inline void gl::Program::bind()
{
    if(!handle)
        return;
    glUseProgram(handle);
}

constexpr inline bool gl::Program::is_separable() const
{
    return separable;
}

constexpr inline GLbitfield gl::Program::get_stage_bits() const
{
    return stage_bits;
}

inline void gl::Program::unbind()
{
    glUseProgram(0);
}

#endif/* CLIENT_RENDER_GL_PROGRAM_HH */
