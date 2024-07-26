// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CAMERA_HH
#define CLIENT_CAMERA_HH
#include <shared/coord.hh>

namespace camera
{
extern unsigned int view_distance;
} // namespace camera

namespace camera
{
void init(void);
void update(void);
} // namespace camera

namespace camera
{
const EntityPos &position(void);
const glm::fvec3 &euler_angles(void);
const glm::fvec3 &direction(void);
const glm::fmat4x4 &matrix(void);
} // namespace camera

#endif /* CLIENT_CAMERA_HH */
