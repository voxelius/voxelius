// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_COORD_HH
#define SHARED_COORD_HH
#include <functional>
#include <shared/vector3D.hh>
#include <shared/vector3Di64.hh>

class ChunkPos;
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

#endif /* SHARED_COORD_HH */
