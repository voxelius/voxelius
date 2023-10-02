// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_EPOCH_HH
#define SHARED_EPOCH_HH
#include <stdint.h>

namespace epoch
{
uint64_t microseconds();
uint64_t milliseconds();
uint64_t seconds();
} // namespace epoch

#endif/* SHARED_EPOCH_HH */
