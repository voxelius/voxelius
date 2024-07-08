// SPDX-License-Identifier: Zlib
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <shared/globals.hh>

entt::dispatcher globals::dispatcher = {};
entt::registry globals::registry = {};
