// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <game/shared/globals.hh>

float globals::frametime = 0.0f;
float globals::frametime_avg = 0.0f;
std::uint64_t globals::frametime_us = 0;
std::uint64_t globals::curtime = 0;
std::uint64_t globals::framecount = 0;

entt::dispatcher globals::dispatcher = {};
entt::registry globals::registry = {};
