// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_RAY_DDA_HH
#define SHARED_RAY_DDA_HH
#include <shared/world.hh>

class RayDDA final {
public:
    Vector3D direction {};
    WorldPos start {};

public:
    Vector3D delta_dist {};
    Vector3D side_dist {};
    VoxelPos vstep {};

public:
    double distance {};
    VoxelPos vnormal {};
    VoxelPos vpos {};

public:
    static void setup(RayDDA &ray, const WorldPos &start, const Vector3D &direction);
    static Voxel step(RayDDA &ray);
};

#endif /* SHARED_RAY_DDA_HH */
