// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef A24B1505_3F78_425C_A682_AF1B5F6E71A4
#define A24B1505_3F78_425C_A682_AF1B5F6E71A4
#include <client/glxx/shader.hh>
#include <shared/identifier.hh>
#include <unordered_map>

using shader_module_t = std::unordered_map<uint32_t, std::string>;

namespace shaders
{
void init();
void add_module(const Identifier &name, const shader_module_t &md);
bool compile(glxx::Shader &shader, const Identifier &path);
} // namespace shaders

#endif/* A24B1505_3F78_425C_A682_AF1B5F6E71A4 */
