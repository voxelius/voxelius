// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <game/shared/chunk_coord.hh>
#include <game/shared/local_coord.hh>
#include <game/shared/world_coord.hh>
#include <game/shared/voxel_coord.hh>

LocalCoord LocalCoord::from_index(std::size_t index)
{
    LocalCoord result = {};
    result[0] = static_cast<std::int64_t>((index % CHUNK_SIZE));
    result[1] = static_cast<std::int64_t>((index / CHUNK_SIZE) / CHUNK_SIZE);
    result[2] = static_cast<std::int64_t>((index / CHUNK_SIZE) % CHUNK_SIZE);
    return std::move(result);
}

std::size_t LocalCoord::to_index(const LocalCoord &lvec)
{
    return static_cast<std::size_t>((lvec[1] * CHUNK_SIZE + lvec[2]) * CHUNK_SIZE + lvec[0]);
}

Vec3f LocalCoord::to_vec3f(const LocalCoord &lvec)
{
    Vec3f result = {};
    result[0] = static_cast<float>(lvec[0]);
    result[1] = static_cast<float>(lvec[1]);
    result[2] = static_cast<float>(lvec[2]);
    return std::move(result);
}
