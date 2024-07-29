// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_WORLD_COORD_HH
#define SHARED_WORLD_COORD_HH
#include <functional>
#include <shared/math/vector3D.hh>
#include <shared/math/vector3Di64.hh>
#include <shared/world/const.hh>

class ChunkCoord;
class LocalCoord;
class VoxelCoord;
class WorldCoord;

class ChunkCoord final : public Vector3Di64 {
public:
    constexpr ChunkCoord(const Vector3Di64 &other);
    using Vector3Di64::Vector3Di64;

public:
    constexpr static VoxelCoord to_voxel(const ChunkCoord &cvec, std::size_t index);
    constexpr static VoxelCoord to_voxel(const ChunkCoord &cvec, const LocalCoord &lvec);
    constexpr static WorldCoord to_world(const ChunkCoord &cvec, const Vector3D &lvec);
    constexpr static WorldCoord to_world(const ChunkCoord &cvec, const LocalCoord &lvec);
    constexpr static Vector3D to_vector3D(const ChunkCoord &cvec);
};

class LocalCoord final : public Vector3Di64 {
public:
    constexpr LocalCoord(const Vector3Di64 &other);
    using Vector3Di64::Vector3Di64;

public:
    constexpr static LocalCoord from_index(const std::size_t index);
    constexpr static std::size_t to_index(const LocalCoord &lvec);
    constexpr static Vector3D to_vector3D(const LocalCoord &lvec);
};

class VoxelCoord final : public Vector3Di64 {
public:
    constexpr VoxelCoord(const Vector3Di64 &other);
    using Vector3Di64::Vector3Di64;

public:
    constexpr static ChunkCoord to_chunk(const VoxelCoord &vvec);
    constexpr static LocalCoord to_local(const VoxelCoord &vvec);
    constexpr static WorldCoord to_world(const VoxelCoord &vvec);
    constexpr static Vector3D to_vector3D(const VoxelCoord &vvec);
};

class WorldCoord final {
public:
    ChunkCoord chunk {};
    Vector3D local {};

public:
    constexpr static LocalCoord to_local(const WorldCoord &wvec);
    constexpr static VoxelCoord to_voxel(const WorldCoord &wvec);
    constexpr static Vector3D to_vector3D(const WorldCoord &wvec);
    constexpr static Vector3D to_vector3D(const WorldCoord &pivot, const ChunkCoord &cvec);
    constexpr static Vector3D to_vector3D(const WorldCoord &pivot, const WorldCoord &wvec);
};

template<>
struct std::hash<ChunkCoord> final {
    constexpr inline std::size_t operator()(const ChunkCoord &cvec) const
    {
        std::size_t value = 0;
        value ^= cvec[0] * 73856093;
        value ^= cvec[1] * 19349663;
        value ^= cvec[2] * 83492791;
        return value;
    }
};

constexpr inline ChunkCoord::ChunkCoord(const Vector3Di64 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
}

constexpr inline VoxelCoord ChunkCoord::to_voxel(const ChunkCoord &cvec, std::size_t index)
{
    return std::move(ChunkCoord::to_voxel(cvec, LocalCoord::from_index(index)));
}

constexpr inline VoxelCoord ChunkCoord::to_voxel(const ChunkCoord &cvec, const LocalCoord &lvec)
{
    VoxelCoord result = {};
    result[0] = lvec[0] + (cvec[0] << CHUNK_SIZE_LOG2);
    result[1] = lvec[1] + (cvec[1] << CHUNK_SIZE_LOG2);
    result[2] = lvec[2] + (cvec[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

constexpr inline WorldCoord ChunkCoord::to_world(const ChunkCoord &cvec, const Vector3D &lvec)
{
    WorldCoord result = {};
    result.chunk = cvec;
    result.local = lvec;
    return std::move(result);
}

constexpr inline WorldCoord ChunkCoord::to_world(const ChunkCoord &cvec, const LocalCoord &lvec)
{
    WorldCoord result = {};
    result.chunk = cvec;
    result.local[0] = static_cast<float>(lvec[0]);
    result.local[1] = static_cast<float>(lvec[1]);
    result.local[2] = static_cast<float>(lvec[2]);
    return std::move(result);
}

constexpr inline Vector3D ChunkCoord::to_vector3D(const ChunkCoord &cvec)
{
    Vector3D result = {};
    result[0] = static_cast<float>(cvec[0] << CHUNK_SIZE_LOG2);
    result[1] = static_cast<float>(cvec[1] << CHUNK_SIZE_LOG2);
    result[2] = static_cast<float>(cvec[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

constexpr inline LocalCoord::LocalCoord(const Vector3Di64 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
}

constexpr inline LocalCoord LocalCoord::from_index(std::size_t index)
{
    LocalCoord result = {};
    result[0] = static_cast<std::int64_t>((index % CHUNK_SIZE));
    result[1] = static_cast<std::int64_t>((index / CHUNK_SIZE) / CHUNK_SIZE);
    result[2] = static_cast<std::int64_t>((index / CHUNK_SIZE) % CHUNK_SIZE);
    return std::move(result);
}

constexpr inline std::size_t LocalCoord::to_index(const LocalCoord &lvec)
{
    return static_cast<std::size_t>((lvec[1] * CHUNK_SIZE + lvec[2]) * CHUNK_SIZE + lvec[0]);
}

constexpr inline Vector3D LocalCoord::to_vector3D(const LocalCoord &lvec)
{
    Vector3D result = {};
    result[0] = static_cast<float>(lvec[0]);
    result[1] = static_cast<float>(lvec[1]);
    result[2] = static_cast<float>(lvec[2]);
    return std::move(result);
}

constexpr inline VoxelCoord::VoxelCoord(const Vector3Di64 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
}

constexpr inline ChunkCoord VoxelCoord::to_chunk(const VoxelCoord &vvec)
{
    ChunkCoord result = {};
    result[0] = vvec[0] >> CHUNK_SIZE_LOG2;
    result[1] = vvec[1] >> CHUNK_SIZE_LOG2;
    result[2] = vvec[2] >> CHUNK_SIZE_LOG2;
    return std::move(result);
}

constexpr inline LocalCoord VoxelCoord::to_local(const VoxelCoord &vvec)
{
    LocalCoord result = {};
    result[0] = vvec[0] % CHUNK_SIZE;
    result[1] = vvec[1] % CHUNK_SIZE;
    result[2] = vvec[2] % CHUNK_SIZE;
    return std::move(result);
}

constexpr inline WorldCoord VoxelCoord::to_world(const VoxelCoord &vvec)
{
    WorldCoord result = {};
    result.chunk[0] = vvec[0] >> CHUNK_SIZE_LOG2;
    result.chunk[1] = vvec[1] >> CHUNK_SIZE_LOG2;
    result.chunk[2] = vvec[2] >> CHUNK_SIZE_LOG2;
    result.local[0] = static_cast<float>(vvec[0] % CHUNK_SIZE);
    result.local[1] = static_cast<float>(vvec[1] % CHUNK_SIZE);
    result.local[2] = static_cast<float>(vvec[2] % CHUNK_SIZE);
    return std::move(result);
}

constexpr inline Vector3D VoxelCoord::to_vector3D(const VoxelCoord &vvec)
{
    Vector3D result = {};
    result[0] = static_cast<float>(vvec[0]);
    result[1] = static_cast<float>(vvec[1]);
    result[2] = static_cast<float>(vvec[2]);
    return std::move(result);
}

constexpr inline LocalCoord WorldCoord::to_local(const WorldCoord &wvec)
{
    LocalCoord result = {};
    result[0] = cxpr::floor<std::int64_t>(wvec.local[0]);
    result[1] = cxpr::floor<std::int64_t>(wvec.local[1]);
    result[2] = cxpr::floor<std::int64_t>(wvec.local[2]);
    return std::move(result);
}

constexpr inline VoxelCoord WorldCoord::to_voxel(const WorldCoord &wvec)
{
    VoxelCoord result = {};
    result[0] = cxpr::floor<std::int64_t>(wvec.local[0]) + (wvec.chunk[0] << CHUNK_SIZE_LOG2);
    result[1] = cxpr::floor<std::int64_t>(wvec.local[1]) + (wvec.chunk[1] << CHUNK_SIZE_LOG2);
    result[2] = cxpr::floor<std::int64_t>(wvec.local[2]) + (wvec.chunk[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

constexpr inline Vector3D WorldCoord::to_vector3D(const WorldCoord &wvec)
{
    Vector3D result = {};
    result[0] = static_cast<float>(wvec.chunk[0] << CHUNK_SIZE_LOG2) + wvec.local[0];
    result[1] = static_cast<float>(wvec.chunk[1] << CHUNK_SIZE_LOG2) + wvec.local[1];
    result[2] = static_cast<float>(wvec.chunk[2] << CHUNK_SIZE_LOG2) + wvec.local[2];
    return std::move(result);
}

constexpr inline Vector3D WorldCoord::to_vector3D(const WorldCoord &pivot, const ChunkCoord &cvec)
{
    Vector3D result = {};
    result[0] = static_cast<float>((cvec[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) - pivot.local[0];
    result[1] = static_cast<float>((cvec[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) - pivot.local[1];
    result[2] = static_cast<float>((cvec[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) - pivot.local[2];
    return std::move(result);
}

constexpr inline Vector3D WorldCoord::to_vector3D(const WorldCoord &pivot, const WorldCoord &wvec)
{
    Vector3D result = {};
    result[0] = static_cast<float>((wvec.chunk[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) + (wvec.local[0] - pivot.local[0]);
    result[1] = static_cast<float>((wvec.chunk[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) + (wvec.local[1] - pivot.local[1]);
    result[2] = static_cast<float>((wvec.chunk[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) + (wvec.local[2] - pivot.local[2]);
    return std::move(result);
}

#endif /* SHARED_WORLD_COORD_HH */
