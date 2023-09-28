// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef C62EC7BF_3CD3_4FE1_92F8_C865267A8E66
#define C62EC7BF_3CD3_4FE1_92F8_C865267A8E66
#include <client/glxx/shader.hh>
#include <shared/vfs.hh>

namespace shaders
{
void init();
void add(const std::string &name, uint32_t stage, const std::string &source);
bool compile(glxx::Shader &shader, const vfs::path_t &path);
} // namespace shaders

#endif/* C62EC7BF_3CD3_4FE1_92F8_C865267A8E66 */
