// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/shared/world_coord.hh>

LocalCoord WorldCoord::to_local(const WorldCoord &wvec)
{
    LocalCoord result = {};
    result[0] = cxpr::floor<std::int64_t>(wvec.local[0]);
    result[1] = cxpr::floor<std::int64_t>(wvec.local[1]);
    result[2] = cxpr::floor<std::int64_t>(wvec.local[2]);
    return std::move(result);
}

VoxelCoord WorldCoord::to_voxel(const WorldCoord &wvec)
{
    VoxelCoord result = {};
    result[0] = cxpr::floor<std::int64_t>(wvec.local[0]) + (wvec.chunk[0] << CHUNK_SIZE_LOG2);
    result[1] = cxpr::floor<std::int64_t>(wvec.local[1]) + (wvec.chunk[1] << CHUNK_SIZE_LOG2);
    result[2] = cxpr::floor<std::int64_t>(wvec.local[2]) + (wvec.chunk[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

Vec3f WorldCoord::to_vec3f(const WorldCoord &wvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>(wvec.chunk[0] << CHUNK_SIZE_LOG2) + wvec.local[0];
    result[1] = static_cast<float>(wvec.chunk[1] << CHUNK_SIZE_LOG2) + wvec.local[1];
    result[2] = static_cast<float>(wvec.chunk[2] << CHUNK_SIZE_LOG2) + wvec.local[2];
    return std::move(result);
}

Vec3f WorldCoord::to_vec3f(const WorldCoord &pivot, const ChunkCoord &cvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>((cvec[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) - pivot.local[0];
    result[1] = static_cast<float>((cvec[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) - pivot.local[1];
    result[2] = static_cast<float>((cvec[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) - pivot.local[2];
    return std::move(result);
}

Vec3f WorldCoord::to_vec3f(const WorldCoord &pivot, const WorldCoord &wvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>((wvec.chunk[0] - pivot.chunk[0]) << CHUNK_SIZE_LOG2) + (wvec.local[0] - pivot.local[0]);
    result[1] = static_cast<float>((wvec.chunk[1] - pivot.chunk[1]) << CHUNK_SIZE_LOG2) + (wvec.local[1] - pivot.local[1]);
    result[2] = static_cast<float>((wvec.chunk[2] - pivot.chunk[2]) << CHUNK_SIZE_LOG2) + (wvec.local[2] - pivot.local[2]);
    return std::move(result);
}
