// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <string>

namespace client_game
{
extern bool vertical_sync;
extern bool world_curvature;
extern unsigned int pixel_size;
extern unsigned int fog_mode;
} // namespace client_game

namespace client_game
{
extern std::string username;
extern std::uint64_t player_uid;
} // namespace client_game


namespace client_game
{
void init(void);
void init_late(void);
void deinit(void);
void update(void);
void update_late(void);
void render(void);
void layout(void);
} // namespace client_game
