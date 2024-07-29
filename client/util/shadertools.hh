// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_UTIL_SHADERTOOLS_HH
#define CLIENT_UTIL_SHADERTOOLS_HH
#include <glad/gl.h>
#include <string>

namespace shadertools
{
GLuint compile_shader(const std::string &path, GLenum shader_stage);
GLuint link_program(GLuint vert, GLuint frag);
} // namespace shadertools

#endif /* CLIENT_UTIL_SHADERTOOLS_HH */
