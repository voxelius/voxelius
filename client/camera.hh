// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CAMERA_HH
#define CLIENT_CAMERA_HH
#include <shared/types.hh>

namespace camera
{
extern float horizontal_fov;
extern unsigned int view_distance;
} // namespace camera

namespace camera
{
void init(void);
void update(void);
} // namespace camera

namespace camera
{
const glm::fvec3 &position(void);
const glm::fvec3 &euler_angles(void);
const glm::fvec3 &direction(void);
const glm::fvec3 &chunk_local(void);
const ChunkPos &chunk_position(void);
const glm::fmat4x4 &matrix(void);
} // namespace camera

#endif /* CLIENT_CAMERA_HH */
