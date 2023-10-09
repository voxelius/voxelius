// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_WORLD_RAYCAST_HH
#define SHARED_WORLD_RAYCAST_HH
#include <shared/world/chunks.hh>

struct RayDDA final {
    vector3_t start {};
    vector3_t direction {};

    Chunk *chunk {nullptr};
    voxel_t voxel {NULL_VOXEL};
    chunk_pos_t cpos {};
    voxel_pos_t vpos {};
    double distance {};
    vector3_t wpos {};

    vector3_t lstep {};
    vector3_t lengths {};
    voxel_pos_t vstep {};
};

namespace raycast
{
bool dda(RayDDA &out, const vector3_t &start, const vector3_t &direction, double max_distance);
} // namespace raycast

#endif /* SHARED_WORLD_RAYCAST_HH */
