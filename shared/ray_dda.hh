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
