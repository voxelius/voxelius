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
void append(int argc, char **argv);
void append(const std::string &option);
void append(const std::string &option, const std::string &argument);
bool get_value(const std::string &option, std::string &argument);
bool contains(const std::string &option);
} // namespace cmdline

#endif /* SHARED_CMDLINE_HH */
