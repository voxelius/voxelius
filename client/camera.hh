// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
