// SPDX-License-Identifier: Zlib
#ifndef SHARED_EPOCH_HH
#define SHARED_EPOCH_HH
#include <stdint.h>

namespace epoch
{
uint64_t microseconds(void);
uint64_t milliseconds(void);
uint64_t seconds(void);
} // namespace epoch

#endif /* SHARED_EPOCH_HH */
