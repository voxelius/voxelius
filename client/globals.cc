// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/gbuffer.hh>
#include <client/globals.hh>
#include <client/glxx/framebuffer.hh>
#include <client/options.hh>
#include <client/ui_screen.hh>

Options globals::options = {};

GLFWwindow *globals::window = nullptr;
int globals::window_width = 0;
int globals::window_height = 0;
double globals::window_aspect = 1.0;

double globals::frametime = 0.0;
double globals::frametime_avg = 0.0;
uint64_t globals::curtime = 0;
uint64_t globals::framecount = 0;

entt::entity globals::player = entt::null;

GBuffer globals::gbuffer_solid = {};
GBuffer globals::gbuffer_cutout = {};
GBuffer globals::gbuffer_blend = {};
glxx::Framebuffer globals::deferred_fbo = {};
glxx::Texture2D globals::deferred_color = {};

canvas::Font globals::font_8px = {};
canvas::Font globals::font_16px = {};

unsigned int globals::ui_scale = 1U;
unsigned int globals::ui_screen = ui::SCREEN_NONE;
