// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/globals.hh>
#include <shared/world.hh>

GLFWwindow *globals::window = nullptr;

float globals::frametime = 0.0f;
float globals::frametime_avg = 0.0f;
uint64_t globals::curtime = 0;
uint64_t globals::framecount = 0;

World globals::world = {};
entt::entity globals::player = entt::null;
