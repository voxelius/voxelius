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

using chunk_pos_t = glm::vec<3, int32_t>;
using local_pos_t = glm::vec<3, int16_t>;
using voxel_pos_t = glm::vec<3, int64_t>;

template<>
struct std::hash<chunk_pos_t> final {
    constexpr inline const size_t operator()(const chunk_pos_t &cpos) const
    {
        return (cpos.x * 73856093) ^ (cpos.y * 19349663) ^ (cpos.z * 83492791);
    }
};

namespace coord
{
constexpr static inline const chunk_pos_t to_chunk(const voxel_pos_t &vpos)
{
    return chunk_pos_t {
        static_cast<chunk_pos_t::value_type>(vpos.x >> CHUNK_SIZE_LOG2),
        static_cast<chunk_pos_t::value_type>(vpos.y >> CHUNK_SIZE_LOG2),
        static_cast<chunk_pos_t::value_type>(vpos.z >> CHUNK_SIZE_LOG2),
    };
}

constexpr static inline const local_pos_t to_local(const voxel_pos_t &vpos)
{
    return local_pos_t {
        glm::abs(static_cast<local_pos_t::value_type>(vpos.x % CHUNK_SIZE)),
        glm::abs(static_cast<local_pos_t::value_type>(vpos.y % CHUNK_SIZE)),
        glm::abs(static_cast<local_pos_t::value_type>(vpos.z % CHUNK_SIZE)),
    };
}

constexpr static inline const local_pos_t to_local(const size_t index)
{
    return local_pos_t {
        static_cast<local_pos_t::value_type>((index / CHUNK_AREA)),
        static_cast<local_pos_t::value_type>((index % CHUNK_SIZE)),
        static_cast<local_pos_t::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE),
    };
}

constexpr static inline const voxel_pos_t to_voxel(const chunk_pos_t &cpos, const local_pos_t &lpos)
{
    return voxel_pos_t {
        static_cast<voxel_pos_t::value_type>(lpos.x + (cpos.x << CHUNK_SIZE_LOG2)),
        static_cast<voxel_pos_t::value_type>(lpos.y + (cpos.y << CHUNK_SIZE_LOG2)),
        static_cast<voxel_pos_t::value_type>(lpos.z + (cpos.z << CHUNK_SIZE_LOG2)),
    };
}

constexpr static inline const voxel_pos_t to_voxel(const glm::dvec3 &wpos)
{
    return voxel_pos_t {
        static_cast<voxel_pos_t::value_type>(wpos.x),
        static_cast<voxel_pos_t::value_type>(wpos.y),
        static_cast<voxel_pos_t::value_type>(wpos.z),
    };
}

constexpr static inline const glm::dvec3 to_world(const chunk_pos_t &cpos)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(cpos.x << CHUNK_SIZE_LOG2),
        static_cast<glm::dvec3::value_type>(cpos.y << CHUNK_SIZE_LOG2),
        static_cast<glm::dvec3::value_type>(cpos.z << CHUNK_SIZE_LOG2),
    };
}

constexpr static inline const glm::dvec3 to_world(const voxel_pos_t &vpos)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(vpos.x),
        static_cast<glm::dvec3::value_type>(vpos.y),
        static_cast<glm::dvec3::value_type>(vpos.z),
    };
}

constexpr static inline const size_t to_index(const local_pos_t &lpos)
{
    return static_cast<size_t>((lpos.x * CHUNK_SIZE + lpos.z) * CHUNK_SIZE + lpos.y);
}
} // namespace coord

#endif /* SHARED_COORD_HH */
