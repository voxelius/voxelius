// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_RAY_DDA_HH
#define SHARED_RAY_DDA_HH
#include <shared/world.hh>

class RayDDA final {
public:
    glm::fvec3 direction {};
    EntityPos start {};

public:
    glm::fvec3 delta_dist {};
    glm::fvec3 side_dist {};
    VoxelPos vstep {};

public:
    double distance {};
    VoxelPos vnormal {};
    VoxelPos vpos {};

public:
    static void setup(RayDDA &ray, const EntityPos &start, const glm::fvec3 &direction);
    static Voxel step(RayDDA &ray);
};

#endif /* SHARED_RAY_DDA_HH */
