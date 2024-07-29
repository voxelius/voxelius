// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_VFSTOOLS_HH
#define SHARED_UTIL_VFSTOOLS_HH
#include <cstdint>
#include <physfs.h>
#include <string>
#include <vector>

namespace vfstools
{
const char *last_error(void);
} // namespace vfstools

namespace vfstools
{
bool read_bytes(const std::string &path, std::vector<std::uint8_t> &buffer);
bool read_string(const std::string &path, std::string &buffer);
bool write_bytes(const std::string &path, const std::vector<std::uint8_t> &buffer);
bool write_string(const std::string &path, const std::string &buffer);
} // namespace vfstools

namespace vfstools
{
bool read_line(PHYSFS_File *file, std::string &line);
} // namespace vfstools

#endif /* SHARED_UTIL_VFSTOOLS_HH */
