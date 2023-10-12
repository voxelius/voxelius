// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_CAMERA_HH
#define CLIENT_CAMERA_HH
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>

namespace camera
{
void update();
const vector3_t &get_position();
const vector3_t &get_euler_anlgles();
const vector3_t &get_direction();
const vector3f_t &get_chunk_local();
const chunk_pos_t &get_chunk_pos();
const matrix4x4f_t &get_matrix();
} // namespace camera

#endif /* CLIENT_CAMERA_HH */
