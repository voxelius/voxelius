// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_EPOCH_HH
#define SHARED_EPOCH_HH
#include <cstdint>

namespace epoch
{
std::uint64_t microseconds(void);
std::uint64_t milliseconds(void);
std::uint64_t seconds(void);
} // namespace epoch

#endif /* SHARED_EPOCH_HH */
