// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_CHUNK_POS_HH
#define SHARED_CHUNK_POS_HH
#include <functional>
#include <shared/vector3D.hh>
#include <shared/vector3Di64.hh>

class LocalPos;
class VoxelPos;
class WorldPos;

class ChunkPos final : public Vector3Di64 {
public:
    ChunkPos(const Vector3D &other);
    ChunkPos(const Vector3Di64 &other);
    using Vector3Di64::Vector3Di64;

public:
    static VoxelPos to_voxel(const ChunkPos &cpos, std::size_t index);
    static VoxelPos to_voxel(const ChunkPos &cpos, const LocalPos &lpos);
    static WorldPos to_world(const ChunkPos &cpos, const Vector3D &lpos);
    static WorldPos to_world(const ChunkPos &cpos, const LocalPos &lpos);
    static Vector3D to_vector3D(const ChunkPos &cpos);
};

template<>
struct std::hash<ChunkPos> final {
    constexpr inline std::size_t operator()(const ChunkPos &vector) const
    {
        std::size_t value = 0;
        value ^= vector[0] * 73856093;
        value ^= vector[1] * 19349663;
        value ^= vector[2] * 83492791;
        return value;
    }
};

#endif /* SHARED_CHUNK_POS_HH */
