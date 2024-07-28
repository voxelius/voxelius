// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_VOXEL_POS_HH
#define SHARED_VOXEL_POS_HH
#include <shared/vector3D.hh>
#include <shared/vector3Di64.hh>

class ChunkPos;
class LocalPos;
class WorldPos;

class VoxelPos final : public Vector3Di64 {
public:
    VoxelPos(const Vector3D &other);
    VoxelPos(const Vector3Di64 &other);
    using Vector3Di64::Vector3Di64;

public:
    static ChunkPos to_chunk(const VoxelPos &vpos);
    static LocalPos to_local(const VoxelPos &vpos);
    static WorldPos to_world(const VoxelPos &vpos);
    static Vector3D to_vector3D(const VoxelPos &vpos);
};

#endif /* SHARED_VOXEL_POS_HH */
