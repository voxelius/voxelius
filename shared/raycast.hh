// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_RAYCAST_HH
#define SHARED_RAYCAST_HH
#include <shared/chunk.hh>
#include <shared/coord.hh>

struct RayDDA final {
    glm::dvec3 start {};
    glm::dvec3 direction {};

    Chunk *chunk {nullptr};
    voxel_t voxel {NULL_VOXEL};
    chunk_pos_t cpos {};
    voxel_pos_t vpos {};
    double distance {};
    glm::dvec3 wpos {};

    glm::dvec3 lstep {};
    glm::dvec3 lengths {};
    voxel_pos_t vstep {};
};

namespace raycast
{
bool dda(RayDDA &out, const glm::dvec3 &start, const glm::dvec3 &direction, double max_distance);
} // namespace raycast

#endif /* SHARED_RAYCAST_HH */
