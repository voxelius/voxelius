// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_WORLD_RAY_DDA_HH
#define SHARED_WORLD_RAY_DDA_HH
#include <shared/world/coord.hh>
#include <shared/world/voxel.hh>

class RayDDA final {
public:
    Vec3f direction {};
    WorldCoord start {};

public:
    Vec3f delta_dist {};
    Vec3f side_dist {};
    VoxelCoord vstep {};

public:
    double distance {};
    VoxelCoord vnormal {};
    VoxelCoord vpos {};

public:
    static void setup(RayDDA &ray, const WorldCoord &start, const Vec3f &direction);
    static Voxel step(RayDDA &ray);
};

#endif /* SHARED_WORLD_RAY_DDA_HH */
