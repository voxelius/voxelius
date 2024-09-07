// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstddef>
#include <cstdint>
#include <enet/enet.h>
#include <entt/entity/entity.hpp>
#include <game/shared/globals.hh>
#include <glad/gl.h>
#include <string>

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

extern ENetHost *client_host;

extern ENetPeer *session_peer;
extern std::uint16_t session_id;
extern std::uint64_t session_tick_dt;
extern std::uint64_t session_send_time;
extern std::string session_username;

extern entt::entity player;

extern GLuint world_fbo;
extern GLuint world_fbo_color;
extern GLuint world_fbo_depth;

extern ImFont *font_chat;
extern ImFont *font_title;
extern ImFont *font_debug;

extern int *gui_keybind_ptr;
extern unsigned int gui_scale;
extern unsigned int gui_screen;
} // namespace globals
