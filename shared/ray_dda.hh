// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_RAY_DDA_HH
#define SHARED_RAY_DDA_HH
#include <shared/chunk.hh>
#include <shared/coord.hh>

class RayDDA final {
public:
    RayDDA() = default;
    RayDDA(const glm::dvec3 &start, const glm::dvec3 &direction);
    RayDDA(const glm::dvec3 &start, const glm::dvec3 &direction, double max_distance);
    bool propagate(double max_distance);

public:
    glm::dvec3 start {};
    glm::dvec3 direction {};

public:
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

#endif /* SHARED_RAY_DDA_HH */
