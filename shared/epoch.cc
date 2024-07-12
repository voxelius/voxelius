// SPDX-License-Identifier: Zlib
#include <chrono>
#include <shared/epoch.hh>

std::uint64_t epoch::microseconds(void)
{
    const auto tv = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(tv).count());
}

std::uint64_t epoch::milliseconds(void)
{
    const auto tv = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(tv).count());
}

std::uint64_t epoch::seconds(void)
{
    const auto tv = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(tv).count());
}
