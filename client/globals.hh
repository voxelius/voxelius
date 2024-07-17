// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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
