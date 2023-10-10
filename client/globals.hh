// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLOBALS_HH
#define CLIENT_GLOBALS_HH
#include <shared/globals.hh>
#include <shared/types.hh>

namespace glxx
{
class Framebuffer;
class Texture2D;
} // namespace glxx

namespace canvas
{
class Font;    
} // namespace canvas

struct GLFWwindow;
class GBuffer;
class World;

namespace globals
{
extern GLFWwindow *window;
extern int window_width;
extern int window_height;
extern double window_aspect;

extern double frametime;
extern double frametime_avg;
extern uint64_t curtime;
extern uint64_t framecount;

extern entt::entity player;

extern GBuffer gbuffer_solid;
extern GBuffer gbuffer_cutout;
extern GBuffer gbuffer_blend;
extern glxx::Framebuffer deferred_fbo;
extern glxx::Texture2D deferred_color;

extern canvas::Font font_8px;
extern canvas::Font font_16px;

extern double ui_scale; // UNDONE: settings

extern unsigned int gameui_screen;
} // namespace globals

#endif /* CLIENT_GLOBALS_HH */
