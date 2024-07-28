// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_LOCAL_POS_HH
#define SHARED_LOCAL_POS_HH
#include <shared/vector3D.hh>
#include <shared/vector3Di64.hh>

class ChunkPos;
class VoxelPos;
class WorldPos;

class LocalPos final : public Vector3Di64 {
public:
    LocalPos(const Vector3D &other);
    LocalPos(const Vector3Di64 &other);
    using Vector3Di64::Vector3Di64;

public:
    static LocalPos from_index(const std::size_t index);
    static std::size_t to_index(const LocalPos &lpos);
    static Vector3D to_vector3D(const LocalPos &lpos);
};

#endif /* SHARED_LOCAL_POS_HH */
