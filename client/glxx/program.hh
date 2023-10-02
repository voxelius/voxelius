// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLXX_PROGRAM_HH
#define CLIENT_GLXX_PROGRAM_HH
#include <array>
#include <client/glxx/shader.hh>
#include <spdlog/spdlog.h>

namespace glxx
{
class Program final : public Object<Program> {
public:
    Program() = default;
    Program(Program &&rhs);
    Program &operator=(Program &&rhs);
    void create();
    void destroy();
    void set_separable(bool separable);
    void attach(const glxx::Shader &shader);
    bool link();
    void bind();
    constexpr bool is_separable() const;
    constexpr GLbitfield get_stage_bits() const;
    static void unbind();

private:
    bool separable {false};
    GLbitfield stage_bits {0};
};

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
} // namespace glxx

inline glxx::Program::Program(glxx::Program &&rhs)
    : separable{rhs.separable}, stage_bits{rhs.stage_bits}
{
    handle = rhs.handle;
    rhs.handle = 0;
    rhs.separable = false;
    rhs.stage_bits = 0;
}

inline glxx::Program &glxx::Program::operator=(glxx::Program &&rhs)
{
    glxx::Program copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    std::swap(separable, copy.separable);
    std::swap(stage_bits, copy.stage_bits);
    return *this;
}

inline void glxx::Program::create()
{
    destroy();
    handle = glCreateProgram();
    glProgramParameteri(handle, GL_PROGRAM_SEPARABLE, separable ? GL_TRUE : GL_FALSE);
}

inline void glxx::Program::destroy()
{
    if(handle) {
        glDeleteProgram(handle);
        stage_bits = 0;
        handle = 0;
    }
}

inline void glxx::Program::set_separable(bool separable)
{
    if(handle)
        glProgramParameteri(handle, GL_PROGRAM_SEPARABLE, separable ? GL_TRUE : GL_FALSE);
    this->separable = separable;
}

inline void glxx::Program::attach(const glxx::Shader &shader)
{
    const GLbitfield stage_bit = detail::get_stage_bit(shader.get_stage());
    if(handle && !(stage_bits & stage_bit)) {
        glAttachShader(handle, shader.get());
        stage_bits |= stage_bit;
    }
}

inline bool glxx::Program::link()
{
    if(handle && stage_bits) {
        int32_t status;

        glLinkProgram(handle);
        glxx::detail::check_program_info_log(handle);

        glGetProgramiv(handle, GL_LINK_STATUS, &status);
        return status;
    }

    return false;
}

inline void glxx::Program::bind()
{
    if(!handle)
        return;
    glUseProgram(handle);
}

constexpr inline bool glxx::Program::is_separable() const
{
    return separable;
}

constexpr inline GLbitfield glxx::Program::get_stage_bits() const
{
    return stage_bits;
}

inline void glxx::Program::unbind()
{
    glUseProgram(0);
}

#endif /* CLIENT_GLXX_PROGRAM_HH */
