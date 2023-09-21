// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B3BD20F9_18D1_4774_A219_C83FBB696B2A
#define B3BD20F9_18D1_4774_A219_C83FBB696B2A
#include <entt/entity/entity.hpp>
#include <GLFW/glfw3.h>
#include <game/shared/globals.hh>
#include <game/shared/world.hh>

namespace globals
{
extern GLFWwindow *window;
extern float frametime;
extern float frametime_avg;
extern uint64_t curtime;
extern uint64_t framecount;
extern entt::entity player;
extern World world;
} // namespace globals

#endif /* B3BD20F9_18D1_4774_A219_C83FBB696B2A */
