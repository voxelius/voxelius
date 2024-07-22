// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <chrono>
#include <shared/util/epoch.hh>

std::uint64_t util::epoch_microseconds(void)
{
    const auto tv = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(tv).count());
}

std::uint64_t util::epoch_milliseconds(void)
{
    const auto tv = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(tv).count());
}

std::uint64_t util::epoch_seconds(void)
{
    const auto tv = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(tv).count());
}
