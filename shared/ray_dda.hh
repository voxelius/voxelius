// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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
    Voxel voxel {NULL_VOXEL};
    ChunkCoord cv {};
    VoxelCoord vv {};
    double distance {};
    glm::dvec3 wpos {};
    glm::dvec3 lstep {};
    glm::dvec3 lengths {};
    VoxelCoord vstep {};
};

#endif /* SHARED_RAY_DDA_HH */
