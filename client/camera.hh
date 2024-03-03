// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef CLIENT_CAMERA_HH
#define CLIENT_CAMERA_HH
#include <entt/entity/entity.hpp>
#include <shared/config/variable.hh>
#include <shared/coord.hh>

namespace camera
{
extern config::Number<double> fov;
extern config::Number<unsigned int> view_distance;
} // namespace camera

namespace camera
{
void init();
void update();
const glm::dvec3 &get_position();
const glm::dvec3 &get_euler_anlgles();
const glm::dvec3 &get_direction();
const glm::fvec3 &get_chunk_local();
const chunk_pos_t &get_chunk_pos();
const glm::fmat4x4 &get_matrix();
} // namespace camera

#endif /* CLIENT_CAMERA_HH */
