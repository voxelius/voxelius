// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_WORLD_POS_HH
#define SHARED_WORLD_POS_HH
#include <shared/chunk_pos.hh>

class ChunkPos;
class LocalPos;
class VoxelPos;
class WorldPos;

class WorldPos final {
public:
    ChunkPos chunk {};
    Vector3D local {};

public:
    static LocalPos to_local(const WorldPos &wpos);
    static VoxelPos to_voxel(const WorldPos &wpos);
    static Vector3D to_vector3D(const WorldPos &wpos);
    static Vector3D to_vector3D(const WorldPos &pivot, const ChunkPos &cpos);
    static Vector3D to_vector3D(const WorldPos &pivot, const WorldPos &wpos);
};

#endif /* SHARED_WORLD_POS_HH */
