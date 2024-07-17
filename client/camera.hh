// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef CLIENT_CAMERA_HH
#define CLIENT_CAMERA_HH
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>

namespace camera
{
extern double fov;
extern unsigned int view_distance;
} // namespace camera

namespace camera
{
void init();
void update();
const glm::dvec3 &get_position();
const glm::dvec3 &get_euler_anlgles();
const glm::dvec3 &get_direction();
const glm::fvec3 &get_chunk_local();
const ChunkCoord &get_chunk_pos();
const glm::fmat4x4 &get_matrix();
} // namespace camera

#endif /* CLIENT_CAMERA_HH */
