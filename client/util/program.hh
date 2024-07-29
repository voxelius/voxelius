// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_UTIL_PROGRAM_HH
#define CLIENT_UTIL_PROGRAM_HH
#include <glad/gl.h>

namespace util
{
GLuint link_program(GLuint vert, GLuint frag);
} // namespace util

#endif /* CLIENT_UTIL_PROGRAM_HH */
