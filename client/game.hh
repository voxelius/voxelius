// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GAME_HH
#define CLIENT_GAME_HH
#include <shared/config/variable.hh>

namespace client_game
{
extern config::Boolean menu_background;
extern config::Number<unsigned int> pixel_size;
extern config::String username;
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
