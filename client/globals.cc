// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/ui_font.hh>
#include <shared/world.hh>

GLFWwindow *globals::window = nullptr;
int globals::window_width = 0;
int globals::window_height = 0;
double globals::window_aspect = 1.0;

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

ui::Font globals::unifont_16x16 = {};
ui::Font globals::pc_vga_8x16 = {};
ui::Font globals::pc_vga_8x8 = {};

unsigned int globals::gameui_scale = 1U;
GameUiScreen globals::gameui_screen = GameUiScreen::Undefined;
bool globals::gameui_visible = true;
