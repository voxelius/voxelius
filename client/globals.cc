// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gameui/screen.hh>
#include <client/globals.hh>
#include <shared/config.hh>

Config globals::client_config = {};

GLFWwindow *globals::window = nullptr;

int globals::width = 0;
int globals::height = 0;
float globals::aspect = 0.0f;

float globals::frametime = 0.0f;
float globals::frametime_avg = 0.0f;
std::uint64_t globals::curtime = 0;
std::uint64_t globals::framecount = 0;
std::size_t globals::num_triangles = 0;

entt::entity globals::player = entt::null;

GLuint globals::world_fbo = 0;
GLuint globals::world_fbo_color = 0;
GLuint globals::world_fbo_depth = 0;

ImFont *globals::font_debug = nullptr;
ImFont *globals::font_menu_title = nullptr;
ImFont *globals::font_menu_button = nullptr;

int *globals::ui_keybind_ptr = nullptr;
unsigned int globals::ui_scale = 0U;
unsigned int globals::ui_screen = UI_SCREEN_NONE;
