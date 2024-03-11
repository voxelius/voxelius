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
#ifndef SHARED_CONFIG_HH
#define SHARED_CONFIG_HH
#include <string>

namespace config
{
void add(const std::string &name, int &vref);
void add(const std::string &name, bool &vref);
void add(const std::string &name, double &vref);
void add(const std::string &name, unsigned &vref);
void add(const std::string &name, std::string &vref);
void load(const std::string &path);
void save(const std::string &path);
} // namespace config

#endif /* SHARED_CONFIG_HH */
