// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef CLIENT_GLOBALS_HH
#define CLIENT_GLOBALS_HH
#include <entt/entity/entity.hpp>
#include <shared/globals.hh>

namespace glxx
{
class Framebuffer;
class Texture2D;
class Renderbuffer;
} // namespace glxx

struct GLFWwindow;
struct ImFont;
class World;

namespace globals
{
extern GLFWwindow *window;

extern int width;
extern int height;
extern double aspect;

extern double frametime;
extern double frametime_avg;
extern uint64_t curtime;
extern uint64_t framecount;

extern entt::entity player;

extern glxx::Framebuffer world_fbo;
extern glxx::Texture2D world_fbo_color;
extern glxx::Renderbuffer world_fbo_depth;

extern ImFont *font_menu_title;
extern ImFont *font_menu_button;

extern unsigned int ui_scale;
extern unsigned int ui_screen;
} // namespace globals

#endif /* CLIENT_GLOBALS_HH */
