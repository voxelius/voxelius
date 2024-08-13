// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SERVER_GAME_HH
#define SERVER_GAME_HH

namespace server_game
{
void init(void);
void init_late(void);
void deinit(void);
void update(void);
void update_late(void);
} // namespace server_game

#endif /* SERVER_GAME_HH */
