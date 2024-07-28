// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/chunk_pos.hh>
#include <shared/const.hh>
#include <shared/local_pos.hh>
#include <shared/voxel_pos.hh>
#include <shared/world_pos.hh>
#include <utility>

VoxelPos::VoxelPos(const Vector3D &other)
{
    this[0][0] = util::floor<std::int64_t>(other[0]);
    this[0][1] = util::floor<std::int64_t>(other[1]);
    this[0][2] = util::floor<std::int64_t>(other[2]);
}

VoxelPos::VoxelPos(const Vector3Di64 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
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
