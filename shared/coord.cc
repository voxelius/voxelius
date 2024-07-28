// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/const.hh>
#include <shared/coord.hh>
#include <utility>

ChunkPos::ChunkPos(const Vector3D &other)
{
    at(0) = util::floor<std::int64_t>(other[0]);
    at(1) = util::floor<std::int64_t>(other[1]);
    at(2) = util::floor<std::int64_t>(other[2]);
}

ChunkPos::ChunkPos(const Vector3Di64 &other)
{
    at(0) = other[0];
    at(1) = other[1];
    at(2) = other[2];
}

VoxelPos ChunkPos::to_voxel(const ChunkPos &cpos, std::size_t index)
{
    return std::move(ChunkPos::to_voxel(cpos, LocalPos::from_index(index)));
}

VoxelPos ChunkPos::to_voxel(const ChunkPos &cpos, const LocalPos &lpos)
{
    VoxelPos result = {};
    result[0] = lpos[0] + (cpos[0] << CHUNK_SIZE_LOG2);
    result[1] = lpos[1] + (cpos[1] << CHUNK_SIZE_LOG2);
    result[2] = lpos[2] + (cpos[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

WorldPos ChunkPos::to_world(const ChunkPos &cpos, const Vector3D &lpos)
{
    WorldPos result = {};
    result.chunk = cpos;
    result.local = lpos;
    return std::move(result);
}

WorldPos ChunkPos::to_world(const ChunkPos &cpos, const LocalPos &lpos)
{
    WorldPos result = {};
    result.chunk = cpos;
    result.local[0] = static_cast<float>(lpos[0]);
    result.local[1] = static_cast<float>(lpos[1]);
    result.local[2] = static_cast<float>(lpos[2]);
    return std::move(result);
}

Vector3D ChunkPos::to_vector3D(const ChunkPos &cpos)
{
    Vector3D result = {};
    result[0] = static_cast<float>(cpos[0] << CHUNK_SIZE_LOG2);
    result[1] = static_cast<float>(cpos[1] << CHUNK_SIZE_LOG2);
    result[2] = static_cast<float>(cpos[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

LocalPos::LocalPos(const Vector3D &other)
{
    at(0) = util::floor<std::int64_t>(other[0]);
    at(1) = util::floor<std::int64_t>(other[1]);
    at(2) = util::floor<std::int64_t>(other[2]);
}

LocalPos::LocalPos(const Vector3Di64 &other)
{
    at(0) = other[0];
    at(1) = other[1];
    at(2) = other[2];
}

LocalPos LocalPos::from_index(std::size_t index)
{
    LocalPos result = {};
    result[0] = static_cast<std::int64_t>((index % CHUNK_SIZE));
    result[1] = static_cast<std::int64_t>((index / CHUNK_SIZE) / CHUNK_SIZE);
    result[2] = static_cast<std::int64_t>((index / CHUNK_SIZE) % CHUNK_SIZE);
    return std::move(result);
}

std::size_t LocalPos::to_index(const LocalPos &lpos)
{
    return static_cast<std::size_t>((lpos[1] * CHUNK_SIZE + lpos[2]) * CHUNK_SIZE + lpos[0]);
}

Vector3D LocalPos::to_vector3D(const LocalPos &lpos)
{
    Vector3D result = {};
    result[0] = static_cast<float>(lpos[0]);
    result[1] = static_cast<float>(lpos[1]);
    result[2] = static_cast<float>(lpos[2]);
    return std::move(result);
}

VoxelPos::VoxelPos(const Vector3D &other)
{
    at(0) = util::floor<std::int64_t>(other[0]);
    at(1) = util::floor<std::int64_t>(other[1]);
    at(2) = util::floor<std::int64_t>(other[2]);
}

VoxelPos::VoxelPos(const Vector3Di64 &other)
{
    at(0) = other[0];
    at(1) = other[1];
    at(2) = other[2];
}

ChunkPos VoxelPos::to_chunk(const VoxelPos &vpos)
{
    ChunkPos result = {};
    result[0] = vpos[0] >> CHUNK_SIZE_LOG2;
    result[1] = vpos[1] >> CHUNK_SIZE_LOG2;
    result[2] = vpos[2] >> CHUNK_SIZE_LOG2;
    return std::move(result);
}

LocalPos VoxelPos::to_local(const VoxelPos &vpos)
{
    LocalPos result = {};
    result[0] = vpos[0] % CHUNK_SIZE;
    result[1] = vpos[1] % CHUNK_SIZE;
    result[2] = vpos[2] % CHUNK_SIZE;
    return std::move(result);
}

WorldPos VoxelPos::to_world(const VoxelPos &vpos)
{
    WorldPos result = {};
    result.chunk[0] = vpos[0] >> CHUNK_SIZE_LOG2;
    result.chunk[1] = vpos[1] >> CHUNK_SIZE_LOG2;
    result.chunk[2] = vpos[2] >> CHUNK_SIZE_LOG2;
    result.local[0] = static_cast<float>(vpos[0] % CHUNK_SIZE);
    result.local[1] = static_cast<float>(vpos[1] % CHUNK_SIZE);
    result.local[2] = static_cast<float>(vpos[2] % CHUNK_SIZE);
    return std::move(result);
}

Vector3D VoxelPos::to_vector3D(const VoxelPos &vpos)
{
    Vector3D result = {};
    result[0] = static_cast<float>(vpos[0]);
    result[1] = static_cast<float>(vpos[1]);
    result[2] = static_cast<float>(vpos[2]);
    return std::move(result);
}

LocalPos WorldPos::to_local(const WorldPos &wpos)
{
    LocalPos result = {};
    result[0] = util::floor<std::int64_t>(wpos.local[0]);
    result[1] = util::floor<std::int64_t>(wpos.local[1]);
    result[2] = util::floor<std::int64_t>(wpos.local[2]);
    return std::move(result);
}

VoxelPos WorldPos::to_voxel(const WorldPos &wpos)
{
    VoxelPos result = {};
    result[0] = util::floor<std::int64_t>(wpos.local[0]) + (wpos.chunk[0] << CHUNK_SIZE_LOG2);
    result[1] = util::floor<std::int64_t>(wpos.local[1]) + (wpos.chunk[1] << CHUNK_SIZE_LOG2);
    result[2] = util::floor<std::int64_t>(wpos.local[2]) + (wpos.chunk[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

Vector3D WorldPos::to_vector3D(const WorldPos &wpos)
{
    Vector3D result = {};
    result[0] = static_cast<float>(wpos.chunk[0] << CHUNK_SIZE_LOG2) + wpos.local[0];
    result[1] = static_cast<float>(wpos.chunk[1] << CHUNK_SIZE_LOG2) + wpos.local[1];
    result[2] = static_cast<float>(wpos.chunk[2] << CHUNK_SIZE_LOG2) + wpos.local[2];
    return std::move(result);
}

Vector3D WorldPos::to_vector3D(const WorldPos &pivot, const ChunkPos &cpos)
{
    Vector3D result = {};
    result[0] = static_cast<float>((cpos[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) - pivot.local[0];
    result[1] = static_cast<float>((cpos[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) - pivot.local[1];
    result[2] = static_cast<float>((cpos[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) - pivot.local[2];
    return std::move(result);
}

Vector3D WorldPos::to_vector3D(const WorldPos &pivot, const WorldPos &wpos)
{
    Vector3D result = {};
    result[0] = static_cast<float>((wpos.chunk[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) + (wpos.local[0] - pivot.local[0]);
    result[1] = static_cast<float>((wpos.chunk[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) + (wpos.local[1] - pivot.local[1]);
    result[2] = static_cast<float>((wpos.chunk[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) + (wpos.local[2] - pivot.local[2]);
    return std::move(result);
}

