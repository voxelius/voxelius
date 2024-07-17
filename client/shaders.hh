// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef CLIENT_SHADERS_HH
#define CLIENT_SHADERS_HH
#include <client/glxx/shader.hh>
#include <shared/vfs.hh>

namespace shaders
{
void init();
void add(const std::string &name, uint32_t stage, const std::string &source);
bool compile(glxx::Shader &shader, const vfs::path_t &path);
} // namespace shaders

#endif /* CLIENT_SHADERS_HH */
