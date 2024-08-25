// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <mathlib/vec3f.hh>

namespace player_move
{
void init(void);
void update(void);
} // namespace player_move

namespace player_move
{
void set_direction(const Vec3f &direction);
} // namespace player_move
