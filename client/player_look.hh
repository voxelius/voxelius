// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_PLAYER_LOOK_HH
#define CLIENT_PLAYER_LOOK_HH
#include <shared/config/boolean.hh>
#include <shared/config/number.hh>

namespace player_look
{
extern config::Boolean raw_input;
extern config::Double sensitivity;
} // namespace player_look

namespace player_look
{
void init();
} // namespace player_look

#endif /* CLIENT_PLAYER_LOOK_HH */
