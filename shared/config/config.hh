// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONFIG_CONFIG_HH
#define SHARED_CONFIG_CONFIG_HH
#include <shared/config/variable.hh>
#include <shared/vfs.hh>

namespace config
{
void load_cmdline();
void load_file(const vfs::path_t &path);
void save_file(const vfs::path_t &path);
void add(const std::string &name, IVariable &vref);
} // namespace config

#endif /* SHARED_CONFIG_CONFIG_HH */
