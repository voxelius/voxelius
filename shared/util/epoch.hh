// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_EPOCH_HH
#define SHARED_UTIL_EPOCH_HH
#include <cstdint>

namespace util
{
std::uint64_t microseconds(void);
std::uint64_t milliseconds(void);
std::uint64_t seconds(void);
} // namespace util

#endif /* SHARED_UTIL_EPOCH_HH */
