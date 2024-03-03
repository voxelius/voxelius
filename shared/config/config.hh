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
