// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_STRING_HH
#define SHARED_UTIL_STRING_HH
#include <string>
#include <vector>

namespace util
{
bool contains(const std::string &str, char character);
bool contains(const std::string &str, const std::string &substr);
} // namespace util

namespace util
{
bool is_empty_or_whitespace(const std::string &str);
bool is_whitespace(const std::string &str);
} // namespace util

namespace util
{
std::string join(const std::vector<std::string> &strings, const std::string &separator);
std::vector<std::string> split(const std::string &str, const std::string &separator);
} // namespace util

namespace util
{
std::string trim_whitespace(const std::string &str);
} // namespace util

#endif /* SHARED_UTIL_STRING_HH */
