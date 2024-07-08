// SPDX-License-Identifier: Zlib
#ifndef SHARED_CRC64_HH
#define SHARED_CRC64_HH
#include <stdint.h>
#include <string>
#include <vector>

namespace crc64
{
uint64_t get(const void *data, size_t size, uint64_t combine = 0);
uint64_t get(const std::vector<uint8_t> &data, uint64_t combine = 0);
uint64_t get(const std::string &data, uint64_t combine = 0);
} // namespace crc64

#endif /* SHARED_CRC64_HH */
