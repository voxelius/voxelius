// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef CLIENT_GAME_HH
#define CLIENT_GAME_HH

namespace client_game
{
void init(void);
void init_late(void);
void shutdown(void);

void update(void);
void update_late(void);

void render(void);
void layout(void);
} // namespace client_game

#endif /* CLIENT_GAME_HH */
