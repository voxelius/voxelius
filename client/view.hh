// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VIEW_HH
#define CLIENT_VIEW_HH
#include <shared/math/euler_angles.hh>
#include <shared/math/matrix4x4.hh>
#include <shared/world/coord.hh>

namespace view
{
extern float vertical_fov;
extern unsigned int max_distance;
} // namespace view

namespace view
{
extern EulerAngles angles;
extern Vector3D direction;
extern Matrix4x4 matrix;
extern WorldCoord position;
} // namespace view

namespace view
{
void init(void);
void update(void);
} // namespace view

#endif /* CLIENT_VIEW_HH */
