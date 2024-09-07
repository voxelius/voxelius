// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/config.hh>
#include <game/client/globals.hh>
#include <game/client/gui_screen.hh>

Config globals::client_config = {};

GLFWwindow *globals::window = nullptr;

int globals::width = 0;
int globals::height = 0;
float globals::aspect = 0.0f;

std::size_t globals::num_drawcalls = 0;
std::size_t globals::num_triangles = 0;

ENetHost *globals::client_host = nullptr;

ENetPeer *globals::session_peer = nullptr;
std::uint16_t globals::session_id = UINT16_MAX;
std::uint64_t globals::session_tick_dt = UINT64_MAX;
std::uint64_t globals::session_send_time = UINT64_MAX;
std::string globals::session_username = std::string();

entt::entity globals::player = entt::null;

GLuint globals::world_fbo = 0;
GLuint globals::world_fbo_color = 0;
GLuint globals::world_fbo_depth = 0;

ImFont *globals::font_chat = nullptr;
ImFont *globals::font_title = nullptr;
ImFont *globals::font_debug = nullptr;

int *globals::gui_keybind_ptr = nullptr;
unsigned int globals::gui_scale = 0U;
unsigned int globals::gui_screen = GUI_SCREEN_NONE;
