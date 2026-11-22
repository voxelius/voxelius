#include "shared/pch.hh"

#include "shared/globals.hh"

entt::dispatcher globals::dispatcher;

std::uint64_t globals::curtime_us;
std::uint64_t globals::current_tick;

std::size_t globals::fixed_framecount;
std::uint64_t globals::fixed_frametime_us;
float globals::fixed_frametime;
float globals::fixed_frametime_avg;
