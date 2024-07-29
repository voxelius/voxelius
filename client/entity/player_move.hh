// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_ENTITY_PLAYER_MOVE_HH
#define CLIENT_ENTITY_PLAYER_MOVE_HH
#include <shared/math/vector3D.hh>

namespace player_move
{
void init(void);
void update(void);
} // namespace player_move

namespace player_move
{
// This can be either set by keyboard input
// or by a gamepad axes mapped to player movement
void set_direction(const Vector3D &direction);
} // namespace player_move

#endif /* CLIENT_ENTITY_PLAYER_MOVE_HH */
