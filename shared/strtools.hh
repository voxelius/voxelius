// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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
std::size_t split(const std::string &str, const std::string &separator, std::vector<std::string> &out);
std::string trim_whitespace(const std::string &str);
} // namespace strtools

#endif /* SHARED_STRTOOLS_HH */
