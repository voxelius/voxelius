// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_GLOBALS_HH
#define SHARED_GLOBALS_HH
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

namespace globals
{
extern entt::registry registry;
extern entt::dispatcher dispatcher;
} // namespace globals

#endif /* SHARED_GLOBALS_HH */
