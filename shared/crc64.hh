// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_CRC64_HH
#define SHARED_CRC64_HH
#include <cstdint>
#include <string>
#include <vector>

namespace crc64
{
std::uint64_t get(const void *data, std::size_t size, std::uint64_t combine = 0);
std::uint64_t get(const std::vector<std::uint8_t> &data, std::uint64_t combine = 0);
std::uint64_t get(const std::string &data, std::uint64_t combine = 0);
} // namespace crc64

#endif /* SHARED_CRC64_HH */
