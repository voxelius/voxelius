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
#ifndef SHARED_STRTOOLS_HH
#define SHARED_STRTOOLS_HH
#include <string>
#include <vector>

namespace strtools
{
bool contains(const std::string &str, char character);
bool contains(const std::string &str, const std::string &substr);
bool is_empty_or_whitespace(const std::string &str);
bool is_whitespace(const std::string &str);
size_t split(const std::string &str, const std::string &separator, std::vector<std::string> &out);
} // namespace strtools

#endif /* SHARED_STRTOOLS_HH */
