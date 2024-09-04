// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <game/shared/chunk_coord.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/voxel_coord.hh>

class WorldCoord final {
public:
    ChunkCoord chunk {};
    Vec3f local {};

public:
    static LocalCoord to_local(const WorldCoord &wvec);
    static VoxelCoord to_voxel(const WorldCoord &wvec);
    static Vec3f to_vec3f(const WorldCoord &wvec);
    static Vec3f to_vec3f(const WorldCoord &pivot, const ChunkCoord &cvec);
    static Vec3f to_vec3f(const WorldCoord &pivot, const WorldCoord &wvec);
};
