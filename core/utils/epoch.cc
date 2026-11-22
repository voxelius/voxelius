#include "core/pch.hh"

#include "core/utils/epoch.hh"

std::uint64_t utils::epoch_seconds(void)
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed);

    return static_cast<std::uint64_t>(seconds.count());
}

std::uint64_t utils::epoch_milliseconds(void)
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    return static_cast<std::uint64_t>(milliseconds.count());
}

std::uint64_t utils::epoch_microseconds(void)
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);

    return static_cast<std::uint64_t>(microseconds.count());
}

std::int64_t utils::signed_epoch_seconds(void)
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed);

    return static_cast<std::int64_t>(seconds.count());
}

std::int64_t utils::signed_epoch_milliseconds(void)
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    return static_cast<std::int64_t>(milliseconds.count());
}

std::int64_t utils::signed_epoch_microseconds(void)
{
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::system_clock::duration(now.time_since_epoch());
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);

    return static_cast<std::int64_t>(microseconds.count());
}
