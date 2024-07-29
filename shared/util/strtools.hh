// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_STRTOOLS_HH
#define SHARED_UTIL_STRTOOLS_HH
#include <string>
#include <vector>

namespace strtools
{
bool contains(const std::string &str, char character);
bool contains(const std::string &str, const std::string &substr);
} // namespace strtools

namespace strtools
{
bool is_empty_or_whitespace(const std::string &str);
bool is_whitespace(const std::string &str);
} // namespace strtools

namespace strtools
{
std::vector<std::string> split(const std::string &str, const std::string &separator);
} // namespace strtools

namespace strtools
{
std::string trim_whitespace(const std::string &str);
} // namespace strtools

#endif /* SHARED_UTIL_STRTOOLS_HH */
