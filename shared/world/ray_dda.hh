// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_WORLD_RAY_DDA_HH
#define SHARED_WORLD_RAY_DDA_HH
#include <shared/world/coord.hh>
#include <shared/world/voxel.hh>

class RayDDA final {
public:
    Vector3D direction {};
    WorldCoord start {};

public:
    Vector3D delta_dist {};
    Vector3D side_dist {};
    VoxelCoord vstep {};

public:
    double distance {};
    VoxelCoord vnormal {};
    VoxelCoord vpos {};

public:
    static void setup(RayDDA &ray, const WorldCoord &start, const Vector3D &direction);
    static Voxel step(RayDDA &ray);
};

#endif /* SHARED_WORLD_RAY_DDA_HH */
