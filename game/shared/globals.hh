// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <entt/signal/fwd.hpp>

namespace globals
{
extern float frametime;
extern float frametime_avg;
extern std::uint64_t frametime_us;
extern std::uint64_t curtime;
extern std::uint64_t framecount;

extern entt::dispatcher dispatcher;
extern entt::registry registry;
} // namespace globals
