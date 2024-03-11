// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef SHARED_COORD_HH
#define SHARED_COORD_HH
#include <functional>
#include <glm/common.hpp>
#include <shared/const.hh>

namespace coord
{
using Chunk = glm::vec<3, int32_t>;
using Local = glm::vec<3, int16_t>;
using Voxel = glm::vec<3, int64_t>;
} // namespace coord

template<>
struct std::hash<coord::Chunk> final {
    constexpr inline size_t operator()(const coord::Chunk &cvec) const
    {
        size_t value = 0;
        value ^= cvec.x * 73856093;
        value ^= cvec.y * 19349663;
        value ^= cvec.z * 83492791;
        return value;
    }
};

namespace coord
{
constexpr static coord::Chunk to_chunk(const coord::Voxel &vvec);
constexpr static coord::Local to_local(const coord::Voxel &vvec);
constexpr static coord::Local to_local(const size_t index);
constexpr static coord::Voxel to_voxel(const coord::Chunk &cvec, const coord::Local &lvec);
constexpr static coord::Voxel to_voxel(const glm::dvec3 &dvec);
constexpr static glm::dvec3 to_dvec(const coord::Chunk &cvec);
constexpr static glm::dvec3 to_dvec(const coord::Voxel &vvec);
constexpr static size_t to_index(const coord::Local &lvec);
} // namespace coord

constexpr static inline coord::Chunk coord::to_chunk(const coord::Voxel &vvec)
{
    return coord::Chunk {
        static_cast<coord::Chunk::value_type>(vvec.x >> CHUNK_BIT_SHIFT),
        static_cast<coord::Chunk::value_type>(vvec.y >> CHUNK_BIT_SHIFT),
        static_cast<coord::Chunk::value_type>(vvec.z >> CHUNK_BIT_SHIFT),
    };
}

constexpr static inline coord::Local coord::to_local(const coord::Voxel &vvec)
{
    return coord::Local {
        glm::abs(static_cast<coord::Local::value_type>(vvec.x % CHUNK_SIZE)),
        glm::abs(static_cast<coord::Local::value_type>(vvec.y % CHUNK_SIZE)),
        glm::abs(static_cast<coord::Local::value_type>(vvec.z % CHUNK_SIZE)),
    };
}

constexpr static inline coord::Local coord::to_local(const size_t index)
{
    return coord::Local {
        static_cast<coord::Local::value_type>((index / CHUNK_AREA)),
        static_cast<coord::Local::value_type>((index % CHUNK_SIZE)),
        static_cast<coord::Local::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE),
    };
}

constexpr static inline coord::Voxel coord::to_voxel(const coord::Chunk &cvec, const coord::Local &lvec)
{
    return coord::Voxel {
        static_cast<coord::Voxel::value_type>(lvec.x + (cvec.x << CHUNK_BIT_SHIFT)),
        static_cast<coord::Voxel::value_type>(lvec.y + (cvec.y << CHUNK_BIT_SHIFT)),
        static_cast<coord::Voxel::value_type>(lvec.z + (cvec.z << CHUNK_BIT_SHIFT)),
    };
}

constexpr static inline coord::Voxel coord::to_voxel(const glm::dvec3 &dvec)
{
    return coord::Voxel {
        static_cast<coord::Voxel::value_type>(dvec.x),
        static_cast<coord::Voxel::value_type>(dvec.y),
        static_cast<coord::Voxel::value_type>(dvec.z),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const coord::Chunk &cvec)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(cvec.x << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cvec.y << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cvec.z << CHUNK_BIT_SHIFT),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const coord::Voxel &vvec)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(vvec.x),
        static_cast<glm::dvec3::value_type>(vvec.y),
        static_cast<glm::dvec3::value_type>(vvec.z),
    };
}

constexpr static inline size_t coord::to_index(const coord::Local &lvec)
{
    return static_cast<size_t>((lvec.x * CHUNK_SIZE + lvec.z) * CHUNK_SIZE + lvec.y);
}

#endif /* SHARED_COORD_HH */
