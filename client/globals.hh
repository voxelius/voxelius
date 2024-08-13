// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GLOBALS_HH
#define CLIENT_GLOBALS_HH
#include <cstddef>
#include <cstdint>
#include <glad/gl.h>
#include <entt/entity/entity.hpp>
#include <shared/math/vec4f.hh>
#include <shared/globals.hh>

class Config;

struct GLFWwindow;
struct ImFont;

namespace globals
{
extern Config client_config;

extern GLFWwindow *window;

extern int width;
extern int height;
extern float aspect;

extern std::size_t num_drawcalls;
extern std::size_t num_triangles;

extern entt::entity player;

extern GLuint world_fbo;
extern GLuint world_fbo_color;
extern GLuint world_fbo_depth;

extern Vec4f sky_color;

extern ImFont *font_debug;
extern ImFont *font_menu_title;

extern int *gui_keybind_ptr;
extern unsigned int gui_scale;
extern unsigned int gui_screen;
} // namespace globals

#endif /* CLIENT_GLOBALS_HH */
