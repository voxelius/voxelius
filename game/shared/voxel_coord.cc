// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/shared/chunk_coord.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/world_coord.hh>
#include <game/shared/voxel_coord.hh>

ChunkCoord VoxelCoord::to_chunk(const VoxelCoord &vvec)
{
    ChunkCoord result = {};
    result[0] = vvec[0] >> CHUNK_SIZE_LOG2;
    result[1] = vvec[1] >> CHUNK_SIZE_LOG2;
    result[2] = vvec[2] >> CHUNK_SIZE_LOG2;
    return std::move(result);
}

LocalCoord VoxelCoord::to_local(const VoxelCoord &vvec)
{
    LocalCoord result = {};
    result[0] = vvec[0] % CHUNK_SIZE;
    result[1] = vvec[1] % CHUNK_SIZE;
    result[2] = vvec[2] % CHUNK_SIZE;
    return std::move(result);
}

WorldCoord VoxelCoord::to_world(const VoxelCoord &vvec)
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

Vec3f VoxelCoord::to_vec3f(const VoxelCoord &vvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>(vvec[0]);
    result[1] = static_cast<float>(vvec[1]);
    result[2] = static_cast<float>(vvec[2]);
    return std::move(result);
}
