// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/chunk_pos.hh>
#include <shared/const.hh>
#include <shared/local_pos.hh>
#include <shared/voxel_pos.hh>
#include <shared/world_pos.hh>
#include <utility>

LocalPos::LocalPos(const Vector3D &other)
{
    this[0][0] = cxpr::floor<std::int64_t>(other[0]);
    this[0][1] = cxpr::floor<std::int64_t>(other[1]);
    this[0][2] = cxpr::floor<std::int64_t>(other[2]);
}

LocalPos::LocalPos(const Vector3Di64 &other)
{
    this[0][0] = other[0];
    this[0][1] = other[1];
    this[0][2] = other[2];
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
