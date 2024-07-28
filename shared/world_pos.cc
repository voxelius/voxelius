// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/chunk_pos.hh>
#include <shared/const.hh>
#include <shared/local_pos.hh>
#include <shared/voxel_pos.hh>
#include <shared/world_pos.hh>
#include <utility>

LocalPos WorldPos::to_local(const WorldPos &wpos)
{
    LocalPos result = {};
    result[0] = cxpr::floor<std::int64_t>(wpos.local[0]);
    result[1] = cxpr::floor<std::int64_t>(wpos.local[1]);
    result[2] = cxpr::floor<std::int64_t>(wpos.local[2]);
    return std::move(result);
}

VoxelPos WorldPos::to_voxel(const WorldPos &wpos)
{
    VoxelPos result = {};
    result[0] = cxpr::floor<std::int64_t>(wpos.local[0]) + (wpos.chunk[0] << CHUNK_SIZE_LOG2);
    result[1] = cxpr::floor<std::int64_t>(wpos.local[1]) + (wpos.chunk[1] << CHUNK_SIZE_LOG2);
    result[2] = cxpr::floor<std::int64_t>(wpos.local[2]) + (wpos.chunk[2] << CHUNK_SIZE_LOG2);
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
