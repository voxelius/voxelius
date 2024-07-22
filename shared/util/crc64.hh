// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_CRC64_HH
#define SHARED_UTIL_CRC64_HH
#include <cstdint>
#include <string>
#include <vector>

namespace util
{
std::uint64_t crc64(const void *data, std::size_t size, std::uint64_t combine = 0);
std::uint64_t crc64(const std::vector<std::uint8_t> &data, std::uint64_t combine = 0);
std::uint64_t crc64(const std::string &data, std::uint64_t combine = 0);
} // namespace util

#endif /* SHARED_UTIL_CRC64_HH */