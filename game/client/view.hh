// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/coord.hh>
#include <mathlib/mat4x4f.hh>
#include <mathlib/vec3angles.hh>

namespace view
{
extern float vertical_fov;
extern unsigned int max_distance;
} // namespace view

namespace view
{
extern Vec3angles angles;
extern Vec3f direction;
extern Mat4x4f matrix;
extern WorldCoord position;
} // namespace view

namespace view
{
void init(void);
void update(void);
} // namespace view
