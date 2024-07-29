// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_UTIL_SHADER_HH
#define CLIENT_UTIL_SHADER_HH
#include <glad/gl.h>
#include <string>

namespace util
{
GLuint compile_shader(const std::string &path, GLenum shader_stage);
} // namespace util

#endif /* CLIENT_UTIL_SHADER_HH */
