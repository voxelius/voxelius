// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/chunk_pos.hh>
#include <shared/const.hh>
#include <shared/local_pos.hh>
#include <shared/voxel_pos.hh>
#include <shared/world_pos.hh>
#include <utility>

ChunkPos::ChunkPos(const Vector3D &other)
{
    this[0][0] = util::floor<std::int64_t>(other[0]);
    this[0][1] = util::floor<std::int64_t>(other[1]);
    this[0][2] = util::floor<std::int64_t>(other[2]);
}

ChunkPos::ChunkPos(const Vector3Di64 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
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
