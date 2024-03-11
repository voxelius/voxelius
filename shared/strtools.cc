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
#include <algorithm>
#include <ctype.h>
#include <shared/strtools.hh>

bool strtools::contains(const std::string &str, char character)
{
    if(str.find(character) == std::string::npos)
        return false;
    return true;
}

bool strtools::contains(const std::string &str, const std::string &substr)
{
    if(str.find(substr) == std::string::npos)
        return false;
    return true;
}

bool strtools::is_empty_or_whitespace(const std::string &str)
{
    if(str.empty())
        return true;
    return std::all_of(str.cbegin(), str.cend(), &isspace);
}

bool strtools::is_whitespace(const std::string &str)
{
    return std::all_of(str.cbegin(), str.cend(), &isspace);
}

size_t strtools::split(const std::string &str, const std::string &separator, std::vector<std::string> &out)
{
    size_t pos = 0;
    size_t prev = 0;

    out.clear();

    while((pos = str.find(separator, prev)) != std::string::npos) {
        out.push_back(str.substr(prev, pos - prev));
        prev = pos + separator.length();
    }

    if(prev <= str.length())
        out.push_back(str.substr(prev, str.length() - prev));
    return out.size();
}

std::string strtools::trim_whitespace(const std::string &str)
{
    const auto su = str.find_first_not_of(" \t\r\n");
    const auto sv = str.find_last_not_of(" \t\r\n");
    if(su != std::string::npos)
        return str.substr(su, sv - su + 1);
    return std::string{};
}
