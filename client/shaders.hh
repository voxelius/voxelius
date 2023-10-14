// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
