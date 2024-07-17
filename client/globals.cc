// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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

entt::entity globals::player = entt::null;

glxx::Framebuffer globals::world_fbo = {};
glxx::Texture2D globals::world_fbo_color = {};
glxx::Renderbuffer globals::world_fbo_depth = {};

ImFont *globals::font_menu_title = nullptr;
ImFont *globals::font_menu_button = nullptr;

unsigned int globals::ui_scale = 0U;
unsigned int globals::ui_screen = ui::SCREEN_NONE;
