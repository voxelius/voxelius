// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GAME_HH
#define CLIENT_GAME_HH
#include <string>

namespace client_game
{
extern bool vertical_sync;
extern bool world_curvature;
extern std::string username;
extern unsigned int pixel_size;
extern unsigned int fog_mode;
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

#endif /* CLIENT_GAME_HH */
