// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/canvas_font.hh>
#include <client/globals.hh>
#include <client/glxx/framebuffer.hh>
#include <client/ui_screen.hh>

GLFWwindow *globals::window = nullptr;

int globals::width = 0;
int globals::height = 0;
double globals::aspect = 0.0;

double globals::frametime = 0.0;
double globals::frametime_avg = 0.0;
uint64_t globals::curtime = 0;
uint64_t globals::framecount = 0;
size_t globals::vertices_drawn = 0;

entt::entity globals::player = entt::null;

glxx::Framebuffer globals::world_fbo = {};
glxx::Texture2D globals::world_fbo_color = {};
glxx::Renderbuffer globals::world_fbo_depth = {};

canvas::Font globals::font_8px = {};
canvas::Font globals::font_16px = {};

unsigned int globals::ui_scale = 1U;
unsigned int globals::ui_screen = ui::SCREEN_NONE;
