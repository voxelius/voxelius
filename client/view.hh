// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VIEW_HH
#define CLIENT_VIEW_HH
#include <shared/math/vec3angles.hh>
#include <shared/math/mat4x4f.hh>
#include <shared/world/coord.hh>

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

#endif /* CLIENT_VIEW_HH */
