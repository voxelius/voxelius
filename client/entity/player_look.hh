// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_ENTITY_PLAYER_LOOK_HH
#define CLIENT_ENTITY_PLAYER_LOOK_HH
#include <shared/math/vector2D.hh>

namespace player_look
{
// Angle change can come from both mouse movement
// and from gamepad axes mapped to player lookaround
void add_angles(const Vector2D &pitch_yaw);
} // namespace player_look

#endif /* CLIENT_ENTITY_PLAYER_LOOK_HH */
