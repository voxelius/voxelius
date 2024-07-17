// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef CLIENT_GAME_HH
#define CLIENT_GAME_HH
#include <string>

namespace client_game
{
extern bool menu_background;
extern unsigned int pixel_size;
extern std::string username;
} // namespace client_game

namespace client_game
{
void init();
void init_late();
void deinit();
void update();
void update_late();
void render();
void layout();
} // namespace client_game

#endif /* CLIENT_GAME_HH */
