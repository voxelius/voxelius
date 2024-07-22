// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/util/shader.hh>
#include <shared/util/physfs.hh>
#include <spdlog/spdlog.h>

GLuint util::compile_shader(const std::string &path, GLenum shader_stage)
{
    std::string source = {};

    if(!util::read_string(path, source)) {
        spdlog::warn("util::shader: {}: {}", path, util::physfs_error());
        return 0;
    }

    const char *source_cstr = source.c_str();
    GLuint shader = glCreateShader(shader_stage);
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);

    GLint info_log_length;
    std::basic_string<GLchar> info_log = {};
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);

    if(info_log_length >= 2) {
        info_log.resize(info_log_length);
        glGetShaderInfoLog(shader, info_log_length, nullptr, info_log.data());
        spdlog::info("util::shader: {}: shader information:", path);
        spdlog::info(info_log);
    }

    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    if(!compile_status) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
