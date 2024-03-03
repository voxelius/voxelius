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
#ifndef SHARED_CMDLINE_HH
#define SHARED_CMDLINE_HH
#include <string>

namespace cmdline
{
void add(int argc, char **argv);
void add(const std::string &opt);
void add(const std::string &opt, const std::string &arg);
bool get(const std::string &opt, std::string &arg);
bool has(const std::string &opt);
} // namespace cmdline

#endif /* SHARED_CMDLINE_HH */
