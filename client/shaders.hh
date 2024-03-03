// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
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
