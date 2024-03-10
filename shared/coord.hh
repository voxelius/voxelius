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
using chunk = glm::vec<3, int32_t>;
using local = glm::vec<3, int16_t>;
using voxel = glm::vec<3, int64_t>;
} // namespace coord

template<>
struct std::hash<coord::chunk> final {
    constexpr inline size_t operator()(const coord::chunk &cc) const
    {
        size_t value = 0;
        value ^= cc.x * 73856093;
        value ^= cc.y * 19349663;
        value ^= cc.z * 83492791;
        return value;
    }
};

namespace coord
{
constexpr static coord::chunk to_chunk(const coord::voxel &vv);
constexpr static coord::local to_local(const coord::voxel &vv);
constexpr static coord::local to_local(const size_t index);
constexpr static coord::voxel to_voxel(const coord::chunk &cc, const coord::local &ll);
constexpr static coord::voxel to_voxel(const glm::dvec3 &dd);
constexpr static glm::dvec3 to_dvec(const coord::chunk &cc);
constexpr static glm::dvec3 to_dvec(const coord::voxel &vv);
constexpr static size_t to_index(const coord::local &ll);
} // namespace coord

constexpr static inline coord::chunk coord::to_chunk(const coord::voxel &vv)
{
    return coord::chunk {
        static_cast<coord::chunk::value_type>(vv.x >> CHUNK_BIT_SHIFT),
        static_cast<coord::chunk::value_type>(vv.y >> CHUNK_BIT_SHIFT),
        static_cast<coord::chunk::value_type>(vv.z >> CHUNK_BIT_SHIFT),
    };
}

constexpr static inline coord::local coord::to_local(const coord::voxel &vv)
{
    return coord::local {
        glm::abs(static_cast<coord::local::value_type>(vv.x % CHUNK_SIZE)),
        glm::abs(static_cast<coord::local::value_type>(vv.y % CHUNK_SIZE)),
        glm::abs(static_cast<coord::local::value_type>(vv.z % CHUNK_SIZE)),
    };
}

constexpr static inline coord::local coord::to_local(const size_t index)
{
    return coord::local {
        static_cast<coord::local::value_type>((index / CHUNK_AREA)),
        static_cast<coord::local::value_type>((index % CHUNK_SIZE)),
        static_cast<coord::local::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE),
    };
}

constexpr static inline coord::voxel coord::to_voxel(const coord::chunk &cc, const coord::local &ll)
{
    return coord::voxel {
        static_cast<coord::voxel::value_type>(ll.x + (cc.x << CHUNK_BIT_SHIFT)),
        static_cast<coord::voxel::value_type>(ll.y + (cc.y << CHUNK_BIT_SHIFT)),
        static_cast<coord::voxel::value_type>(ll.z + (cc.z << CHUNK_BIT_SHIFT)),
    };
}

constexpr static inline coord::voxel coord::to_voxel(const glm::dvec3 &dd)
{
    return coord::voxel {
        static_cast<coord::voxel::value_type>(dd.x),
        static_cast<coord::voxel::value_type>(dd.y),
        static_cast<coord::voxel::value_type>(dd.z),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const coord::chunk &cc)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(cc.x << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cc.y << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cc.z << CHUNK_BIT_SHIFT),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const coord::voxel &vv)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(vv.x),
        static_cast<glm::dvec3::value_type>(vv.y),
        static_cast<glm::dvec3::value_type>(vv.z),
    };
}

constexpr static inline size_t coord::to_index(const coord::local &ll)
{
    return static_cast<size_t>((ll.x * CHUNK_SIZE + ll.z) * CHUNK_SIZE + ll.y);
}

#endif /* SHARED_COORD_HH */
