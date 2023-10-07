// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLOBALS_HH
#define CLIENT_GLOBALS_HH
#include <entt/fwd.hpp>
#include <shared/globals.hh>
#include <shared/types.hh>

namespace gl
{
class Framebuffer;
class Texture2D;
} // namespace gl

namespace ui
{
class Font;
} // namespace ui

struct GLFWwindow;
class GBuffer;
class World;

namespace globals
{
extern GLFWwindow *window;
extern int window_width;
extern int window_height;
extern double window_aspect;

extern double cursor_xpos;
extern double cursor_ypos;

extern double frametime;
extern double frametime_avg;
extern uint64_t curtime;
extern uint64_t framecount;

extern World world;
extern entt::entity player;

extern GBuffer gbuffer_solid;
extern GBuffer gbuffer_cutout;
extern GBuffer gbuffer_blend;
extern gl::Framebuffer deferred_fbo;
extern gl::Texture2D deferred_color;

extern ui::Font unifont_16x16;
extern ui::Font pc_vga_8x16;
extern ui::Font pc_vga_8x8;
} // namespace globals

#endif/* CLIENT_GLOBALS_HH */
