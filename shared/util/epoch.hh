// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_EPOCH_HH
#define SHARED_UTIL_EPOCH_HH
#include <cstdint>

namespace util
{
std::uint64_t epoch_microseconds(void);
std::uint64_t epoch_milliseconds(void);
std::uint64_t epoch_seconds(void);
} // namespace util

#endif /* SHARED_UTIL_EPOCH_HH */
