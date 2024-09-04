// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/shared/chunk_coord.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/world_coord.hh>
#include <game/shared/voxel_coord.hh>

VoxelCoord ChunkCoord::to_voxel(const ChunkCoord &cvec, std::size_t index)
{
    return std::move(ChunkCoord::to_voxel(cvec, LocalCoord::from_index(index)));
}

VoxelCoord ChunkCoord::to_voxel(const ChunkCoord &cvec, const LocalCoord &lvec)
{
    VoxelCoord result = {};
    result[0] = lvec[0] + (cvec[0] << CHUNK_SIZE_LOG2);
    result[1] = lvec[1] + (cvec[1] << CHUNK_SIZE_LOG2);
    result[2] = lvec[2] + (cvec[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}

WorldCoord ChunkCoord::to_world(const ChunkCoord &cvec, const Vec3f &lvec)
{
    WorldCoord result = {};
    result.chunk = cvec;
    result.local = lvec;
    return std::move(result);
}

WorldCoord ChunkCoord::to_world(const ChunkCoord &cvec, const LocalCoord &lvec)
{
    WorldCoord result = {};
    result.chunk = cvec;
    result.local[0] = static_cast<float>(lvec[0]);
    result.local[1] = static_cast<float>(lvec[1]);
    result.local[2] = static_cast<float>(lvec[2]);
    return std::move(result);
}

WorldCoord ChunkCoord::to_world(const ChunkCoord &cvec)
{
    WorldCoord result = {};
    result.chunk = cvec;
    return std::move(result);
}

Vec3f ChunkCoord::to_vec3f(const ChunkCoord &cvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>(cvec[0] << CHUNK_SIZE_LOG2);
    result[1] = static_cast<float>(cvec[1] << CHUNK_SIZE_LOG2);
    result[2] = static_cast<float>(cvec[2] << CHUNK_SIZE_LOG2);
    return std::move(result);
}
