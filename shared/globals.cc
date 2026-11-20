#include "shared/pch.hh"

#include "shared/globals.hh"

#include "shared/constant.hh"

entt::dispatcher globals::dispatcher;
entt::registry globals::registry;
World* globals::world { nullptr };

std::uint64_t globals::curtime_us;
std::uint64_t globals::current_tick;

// Noon until a server says otherwise, so a menu
// or a client that has not connected yet stays bright
std::uint64_t globals::daylight_tick { constant::DAY_LENGTH_TICKS / 4 };
std::uint64_t globals::day;

std::size_t globals::fixed_framecount;
std::uint64_t globals::fixed_frametime_us;
float globals::fixed_frametime;
float globals::fixed_frametime_avg;
