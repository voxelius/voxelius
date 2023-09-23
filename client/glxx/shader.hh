// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef E29ED32A_C651_432D_BBC4_B9705EC39AB8
#define E29ED32A_C651_432D_BBC4_B9705EC39AB8
#include <client/glxx/object.hh>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace glxx
{
class Shader final : public Object<Shader> {
public:
    Shader() = default;
    Shader(Shader &&rhs);
    Shader &operator=(Shader &&rhs);
    void create(GLenum stage);
    void destroy();
    bool glsl(const std::string &source);
#if 0
    bool spirv(const std::vector<uint8_t> &binary);
#endif
    constexpr GLenum get_stage() const;

private:
    GLenum stage {0};
};

namespace detail
{
static inline void check_shader_info_log(uint32_t shader)
{
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if(length > 1) {
        std::string info_log(static_cast<size_t>(length) + 1, static_cast<char>(0));
        glGetShaderInfoLog(shader, static_cast<GLsizei>(info_log.size()), nullptr, info_log.data());
        spdlog::debug(info_log);
    }
}
} // namespace detail
} // namespace glxx

inline glxx::Shader::Shader(glxx::Shader &&rhs)
    : stage{rhs.stage}
{
    handle = rhs.handle;
    rhs.handle = 0;
    rhs.stage = 0;
}

inline glxx::Shader &glxx::Shader::operator=(glxx::Shader &&rhs)
{
    glxx::Shader copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    std::swap(stage, copy.stage);
    return *this;
}

inline void glxx::Shader::create(GLenum stage)
{
    destroy();
    this->handle = glCreateShader(stage);
    this->stage = stage;
}

inline void glxx::Shader::destroy()
{
    if(handle) {
        glDeleteShader(handle);
        handle = 0;
        stage = 0;
    }
}

inline bool glxx::Shader::glsl(const std::string &source)
{
    if(handle) {
        GLint status;
        const char *data = source.c_str();

        glShaderSource(handle, 1, &data, nullptr);
        glCompileShader(handle);
        glxx::detail::check_shader_info_log(handle);

        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
        return status == GL_TRUE;
    }

    return false;
}

#if 0
inline bool glxx::Shader::spirv(GLenum stage, const std::vector<uint8_t> &binary)
{
    if(handle && stage) {
        GLint status;
        const char *data = source.c_str();

        glShaderBinary(1, &handle, GL_SHADER_BINARY_FORMAT_SPIR_V, binary.data(), static_cast<GLsizei>(binary.size()));
        glSpecializeShader(handle, "main", 0, nullptr, nullptr);
        glxx::detail::check_shader_info_log(handle);

        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
        return status;
    }

    return false;
}
#endif

inline constexpr GLenum glxx::Shader::get_stage() const
{
    return stage;
}

#endif /* E29ED32A_C651_432D_BBC4_B9705EC39AB8 */
