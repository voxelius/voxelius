// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <chrono>
#include <shared/epoch.hh>

uint64_t epoch::microseconds()
{
    const auto curtime = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(curtime).count());
}

uint64_t epoch::milliseconds()
{
    const auto curtime = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(curtime).count());
}

uint64_t epoch::seconds()
{
    const auto curtime = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(curtime).count());
}
