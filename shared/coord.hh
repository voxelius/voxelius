// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef F55EA7C6_AAD7_43D1_AAA1_017426C5544B
#define F55EA7C6_AAD7_43D1_AAA1_017426C5544B
#include <functional>
#include <glm/common.hpp>
#include <shared/const.hh>
#include <stdint.h>

using chunk_coord_t = glm::vec<3, int32_t, glm::packed_highp>;
using local_coord_t = glm::vec<3, int16_t, glm::packed_highp>;
using voxel_coord_t = glm::vec<3, int64_t, glm::packed_highp>;

template<>
struct std::hash<chunk_coord_t> final {
    constexpr inline const size_t operator()(const chunk_coord_t &cvec) const
    {
        return (cvec.x * 73856093) ^ (cvec.y * 19349663) ^ (cvec.z * 83492791);
    }
};

namespace coord
{
constexpr static inline const chunk_coord_t to_chunk(const voxel_coord_t &vvec)
{
    return chunk_coord_t {
        static_cast<chunk_coord_t::value_type>(vvec.x >> CHUNK_SIZE_LOG2),
        static_cast<chunk_coord_t::value_type>(vvec.y >> CHUNK_SIZE_LOG2),
        static_cast<chunk_coord_t::value_type>(vvec.z >> CHUNK_SIZE_LOG2),
    };
}

constexpr static inline const local_coord_t to_local(const voxel_coord_t &vvec)
{
    return local_coord_t {
        glm::abs(static_cast<local_coord_t::value_type>(vvec.x % CHUNK_SIZE)),
        glm::abs(static_cast<local_coord_t::value_type>(vvec.y % CHUNK_SIZE)),
        glm::abs(static_cast<local_coord_t::value_type>(vvec.z % CHUNK_SIZE)),
    };
}

constexpr static inline const local_coord_t to_local(const size_t index)
{
    return local_coord_t {
        static_cast<local_coord_t::value_type>((index / CHUNK_AREA)),
        static_cast<local_coord_t::value_type>((index % CHUNK_SIZE)),
        static_cast<local_coord_t::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE),
    };
}

constexpr static inline const voxel_coord_t to_voxel(const chunk_coord_t &cvec, const local_coord_t &lvec)
{
    return voxel_coord_t {
        static_cast<voxel_coord_t::value_type>(lvec.x + (cvec.x << CHUNK_SIZE_LOG2)),
        static_cast<voxel_coord_t::value_type>(lvec.y + (cvec.y << CHUNK_SIZE_LOG2)),
        static_cast<voxel_coord_t::value_type>(lvec.z + (cvec.z << CHUNK_SIZE_LOG2)),
    };
}

constexpr static inline const voxel_coord_t to_voxel(const glm::vec3 &wvec)
{
    return voxel_coord_t {
        static_cast<voxel_coord_t::value_type>(wvec.x),
        static_cast<voxel_coord_t::value_type>(wvec.y),
        static_cast<voxel_coord_t::value_type>(wvec.z),
    };
}

constexpr static inline const glm::vec3 to_world(const chunk_coord_t &cvec)
{
    return glm::vec3 {
        static_cast<glm::vec3::value_type>(cvec.x << CHUNK_SIZE_LOG2),
        static_cast<glm::vec3::value_type>(cvec.y << CHUNK_SIZE_LOG2),
        static_cast<glm::vec3::value_type>(cvec.z << CHUNK_SIZE_LOG2),
    };
}

constexpr static inline const glm::vec3 to_world(const voxel_coord_t &vvec)
{
    return glm::vec3 {
        static_cast<glm::vec3::value_type>(vvec.x),
        static_cast<glm::vec3::value_type>(vvec.y),
        static_cast<glm::vec3::value_type>(vvec.z),
    };
}

constexpr static inline const size_t to_index(const local_coord_t &lvec)
{
    return static_cast<size_t>((lvec.x * CHUNK_SIZE + lvec.z) * CHUNK_SIZE + lvec.y);
}
} // namespace coord

#endif /* F55EA7C6_AAD7_43D1_AAA1_017426C5544B */
