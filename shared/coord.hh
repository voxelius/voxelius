// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_COORD_HH
#define SHARED_COORD_HH
#include <functional>
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

constexpr static inline const voxel_pos_t to_voxel(const vector3_t &wpos)
{
    return voxel_pos_t {
        static_cast<voxel_pos_t::value_type>(wpos.x),
        static_cast<voxel_pos_t::value_type>(wpos.y),
        static_cast<voxel_pos_t::value_type>(wpos.z),
    };
}

constexpr static inline const vector3_t to_world(const chunk_pos_t &cpos)
{
    return vector3_t {
        static_cast<vector3_t::value_type>(cpos.x << CHUNK_SIZE_LOG2),
        static_cast<vector3_t::value_type>(cpos.y << CHUNK_SIZE_LOG2),
        static_cast<vector3_t::value_type>(cpos.z << CHUNK_SIZE_LOG2),
    };
}

constexpr static inline const vector3_t to_world(const voxel_pos_t &vpos)
{
    return vector3_t {
        static_cast<vector3_t::value_type>(vpos.x),
        static_cast<vector3_t::value_type>(vpos.y),
        static_cast<vector3_t::value_type>(vpos.z),
    };
}

constexpr static inline const size_t to_index(const local_pos_t &lpos)
{
    return static_cast<size_t>((lpos.x * CHUNK_SIZE + lpos.z) * CHUNK_SIZE + lpos.y);
}
} // namespace coord

#endif /* SHARED_COORD_HH */
