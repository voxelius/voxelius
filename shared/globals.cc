// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/globals.hh>

entt::dispatcher globals::dispatcher = {};
entt::registry globals::registry = {};
