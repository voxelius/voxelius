// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GAME_HH
#define CLIENT_GAME_HH

namespace client_game
{
void init();
void init_late();
void deinit();
void update();
void update_late();
void render();
} // namespace client_game

#endif /* CLIENT_GAME_HH */
