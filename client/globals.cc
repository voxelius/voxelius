// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <shared/world.hh>

GLFWwindow *globals::window = nullptr;

double globals::frametime = 0.0;
double globals::frametime_avg = 0.0;
uint64_t globals::curtime = 0;
uint64_t globals::framecount = 0;

World globals::world = {};
entt::entity globals::player = entt::null;

GBuffer globals::gbuffer_solid = {};
GBuffer globals::gbuffer_cutout = {};
GBuffer globals::gbuffer_blend = {};
gl::Framebuffer globals::deferred_fbo = {};
gl::Texture2D globals::deferred_color = {};
