// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <physfs.h>
#include <string>
#include <vector>

namespace fstools
{
const char *error(void);
} // namespace fstools

namespace fstools
{
bool read_bytes(const std::string &path, std::vector<std::uint8_t> &buffer);
bool read_string(const std::string &path, std::string &buffer);
bool write_bytes(const std::string &path, const std::vector<std::uint8_t> &buffer);
bool write_string(const std::string &path, const std::string &buffer);
} // namespace fstools

namespace fstools
{
bool read_line(PHYSFS_File *file, std::string &line);
} // namespace fstools
