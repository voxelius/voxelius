// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <algorithm>
#include <cctype>
#include <shared/util/string.hh>

bool util::contains(const std::string &str, char character)
{
    if(str.find(character) == std::string::npos)
        return false;
    return true;
}

bool util::contains(const std::string &str, const std::string &substr)
{
    if(str.find(substr) == std::string::npos)
        return false;
    return true;
}

bool util::is_empty_or_whitespace(const std::string &str)
{
    if(str.empty())
        return true;
    return std::all_of(str.cbegin(), str.cend(), [](auto c) { return std::isspace(c); });
}

bool util::is_whitespace(const std::string &str)
{
    return std::all_of(str.cbegin(), str.cend(), [](auto c) { return std::isspace(c); });
}

std::vector<std::string> util::split(const std::string &str, const std::string &separator)
{
    std::size_t pos = 0;
    std::size_t prev = 0;
    std::vector<std::string> result = {};

    while((pos = str.find(separator, prev)) != std::string::npos) {
        result.push_back(str.substr(prev, pos - prev));
        prev = pos + separator.length();
    }

    if(prev <= str.length())
        result.push_back(str.substr(prev, str.length() - prev));
    return result;
}

std::string util::trim_whitespace(const std::string &str)
{
    const auto su = str.find_first_not_of(" \t\r\n");
    const auto sv = str.find_last_not_of(" \t\r\n");
    if(su != std::string::npos)
        return str.substr(su, sv - su + 1);
    return std::string();
}
