// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_GLOBALS_HH
#define SHARED_GLOBALS_HH
#include <cstdint>
#include <enet/enet.h>
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

extern ENetHost *host;
} // namespace globals

#endif /* SHARED_GLOBALS_HH */
