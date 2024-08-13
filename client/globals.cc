// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <client/gui_screen.hh>
#include <client/globals.hh>
#include <shared/config.hh>

Config globals::client_config = {};

GLFWwindow *globals::window = nullptr;

int globals::width = 0;
int globals::height = 0;
float globals::aspect = 0.0f;

std::size_t globals::num_drawcalls = 0;
std::size_t globals::num_triangles = 0;

ENetPeer *globlas::peer = nullptr;
ClientState globlas::state = ClientState::Disconnected;
entt::entity globlas::player = entt::null;
std::uint16_t globlas::session_id = UINT16_MAX;

GLuint globals::world_fbo = 0;
GLuint globals::world_fbo_color = 0;
GLuint globals::world_fbo_depth = 0;

Vec4f globals::sky_color = Vec4f::transparent();

ImFont *globals::font_debug = nullptr;
ImFont *globals::font_menu_title = nullptr;

int *globals::gui_keybind_ptr = nullptr;
unsigned int globals::gui_scale = 0U;
unsigned int globals::gui_screen = GUI_SCREEN_NONE;
