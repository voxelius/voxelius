// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_COORD_HH
#define SHARED_COORD_HH
#include <functional>
#include <shared/math/vec3f.hh>
#include <shared/const.hh>

class ChunkCoord;
class LocalCoord;
class VoxelCoord;
class WorldCoord;

class ChunkCoord final : public Vec3base<std::int32_t> {
public:
    template<typename T>
    constexpr ChunkCoord(const Vec3base<T> &other);
    using Vec3base<std::int32_t>::Vec3base;

public:
    constexpr static VoxelCoord to_voxel(const ChunkCoord &cvec, std::size_t index);
    constexpr static VoxelCoord to_voxel(const ChunkCoord &cvec, const LocalCoord &lvec);
    constexpr static WorldCoord to_world(const ChunkCoord &cvec, const Vec3f &lvec);
    constexpr static WorldCoord to_world(const ChunkCoord &cvec, const LocalCoord &lvec);
    constexpr static WorldCoord to_world(const ChunkCoord &cvec);
    constexpr static Vec3f to_vec3f(const ChunkCoord &cvec);
};

class LocalCoord final : public Vec3base<std::int16_t> {
public:
    template<typename T>
    constexpr LocalCoord(const Vec3base<T> &other);
    using Vec3base<std::int16_t>::Vec3base;

public:
    constexpr static LocalCoord from_index(const std::size_t index);
    constexpr static std::size_t to_index(const LocalCoord &lvec);
    constexpr static Vec3f to_vec3f(const LocalCoord &lvec);
};

class VoxelCoord final : public Vec3base<std::int64_t> {
public:
    template<typename T>
    constexpr VoxelCoord(const Vec3base<T> &other);
    using Vec3base<std::int64_t>::Vec3base;

public:
    constexpr static ChunkCoord to_chunk(const VoxelCoord &vvec);
    constexpr static LocalCoord to_local(const VoxelCoord &vvec);
    constexpr static WorldCoord to_world(const VoxelCoord &vvec);
    constexpr static Vec3f to_vec3f(const VoxelCoord &vvec);
};

class WorldCoord final {
public:
    ChunkCoord chunk {};
    Vec3f local {};

public:
    constexpr static LocalCoord to_local(const WorldCoord &wvec);
    constexpr static VoxelCoord to_voxel(const WorldCoord &wvec);
    constexpr static Vec3f to_vec3f(const WorldCoord &wvec);
    constexpr static Vec3f to_vec3f(const WorldCoord &pivot, const ChunkCoord &cvec);
    constexpr static Vec3f to_vec3f(const WorldCoord &pivot, const WorldCoord &wvec);
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

template<typename T>
constexpr inline ChunkCoord::ChunkCoord(const Vec3base<T> &other)
{
    this[0][0] = static_cast<std::int64_t>(other[0]);
    this[0][1] = static_cast<std::int64_t>(other[1]);
    this[0][2] = static_cast<std::int64_t>(other[2]);
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

constexpr inline WorldCoord ChunkCoord::to_world(const ChunkCoord &cvec, const Vec3f &lvec)
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

constexpr inline WorldCoord ChunkCoord::to_world(const ChunkCoord &cvec)
{
    WorldCoord result = {};
    result.chunk = cvec;
    return std::move(result);
}

constexpr inline Vec3f ChunkCoord::to_vec3f(const ChunkCoord &cvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>(cvec[0] << CHUNK_SIZE_LOG2);
    result[1] = static_cast<float>(cvec[1] << CHUNK_SIZE_LOG2);
    result[2] = static_cast<float>(cvec[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

template<typename T>
constexpr inline LocalCoord::LocalCoord(const Vec3base<T> &other)
{
    this[0][0] = static_cast<std::int16_t>(other[0]);
    this[0][1] = static_cast<std::int16_t>(other[1]);
    this[0][2] = static_cast<std::int16_t>(other[2]);
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

constexpr inline Vec3f LocalCoord::to_vec3f(const LocalCoord &lvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>(lvec[0]);
    result[1] = static_cast<float>(lvec[1]);
    result[2] = static_cast<float>(lvec[2]);
    return std::move(result);
}

template<typename T>
constexpr inline VoxelCoord::VoxelCoord(const Vec3base<T> &other)
{
    this[0][0] = static_cast<std::int32_t>(other[0]);
    this[0][1] = static_cast<std::int32_t>(other[1]);
    this[0][2] = static_cast<std::int32_t>(other[2]);
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

constexpr inline Vec3f VoxelCoord::to_vec3f(const VoxelCoord &vvec)
{
    Vec3f result = {};
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

constexpr inline Vec3f WorldCoord::to_vec3f(const WorldCoord &wvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>(wvec.chunk[0] << CHUNK_SIZE_LOG2) + wvec.local[0];
    result[1] = static_cast<float>(wvec.chunk[1] << CHUNK_SIZE_LOG2) + wvec.local[1];
    result[2] = static_cast<float>(wvec.chunk[2] << CHUNK_SIZE_LOG2) + wvec.local[2];
    return std::move(result);
}

constexpr inline Vec3f WorldCoord::to_vec3f(const WorldCoord &pivot, const ChunkCoord &cvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>((cvec[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) - pivot.local[0];
    result[1] = static_cast<float>((cvec[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) - pivot.local[1];
    result[2] = static_cast<float>((cvec[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) - pivot.local[2];
    return std::move(result);
}

constexpr inline Vec3f WorldCoord::to_vec3f(const WorldCoord &pivot, const WorldCoord &wvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>((wvec.chunk[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) + (wvec.local[0] - pivot.local[0]);
    result[1] = static_cast<float>((wvec.chunk[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) + (wvec.local[1] - pivot.local[1]);
    result[2] = static_cast<float>((wvec.chunk[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) + (wvec.local[2] - pivot.local[2]);
    return std::move(result);
}

#endif /* SHARED_COORD_HH */
