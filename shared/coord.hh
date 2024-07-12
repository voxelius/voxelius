// SPDX-License-Identifier: Zlib
#ifndef SHARED_COORD_HH
#define SHARED_COORD_HH
#include <cstddef>
#include <functional>
#include <glm/common.hpp>
#include <shared/const.hh>

using ChunkCoord = glm::vec<3, std::int32_t>;
using LocalCoord = glm::vec<3, std::int16_t>;
using VoxelCoord = glm::vec<3, std::int64_t>;

template<>
struct std::hash<ChunkCoord> final {
    constexpr inline std::size_t operator()(const ChunkCoord &cv) const
    {
        std::size_t value = 0;
        value ^= cv.x * 73856093;
        value ^= cv.y * 19349663;
        value ^= cv.z * 83492791;
        return value;
    }
};

template<>
struct std::hash<VoxelCoord> final {
    constexpr inline std::size_t operator()(const VoxelCoord &vv) const
    {
        std::size_t value = 0;
        value ^= vv.x * 73856093;
        value ^= vv.y * 19349663;
        value ^= vv.z * 83492791;
        return value;
    }
};

namespace coord
{
constexpr static ChunkCoord to_chunk(const VoxelCoord &vv);
constexpr static LocalCoord to_local(const VoxelCoord &vv);
constexpr static LocalCoord to_local(const std::size_t index);
constexpr static VoxelCoord to_voxel(const ChunkCoord &cv, const LocalCoord &lv);
constexpr static VoxelCoord to_voxel(const glm::dvec3 &dv);
constexpr static glm::dvec3 to_dvec(const ChunkCoord &cv);
constexpr static glm::dvec3 to_dvec(const VoxelCoord &vv);
constexpr static std::size_t to_index(const LocalCoord &lv);
} // namespace coord

constexpr static inline ChunkCoord coord::to_chunk(const VoxelCoord &vv)
{
    return ChunkCoord {
        static_cast<ChunkCoord::value_type>(vv.x >> CHUNK_BIT_SHIFT),
        static_cast<ChunkCoord::value_type>(vv.y >> CHUNK_BIT_SHIFT),
        static_cast<ChunkCoord::value_type>(vv.z >> CHUNK_BIT_SHIFT),
    };
}

constexpr static inline LocalCoord coord::to_local(const VoxelCoord &vv)
{
    return LocalCoord {
        glm::abs(static_cast<LocalCoord::value_type>(vv.x % CHUNK_SIZE)),
        glm::abs(static_cast<LocalCoord::value_type>(vv.y % CHUNK_SIZE)),
        glm::abs(static_cast<LocalCoord::value_type>(vv.z % CHUNK_SIZE)),
    };
}

constexpr static inline LocalCoord coord::to_local(const std::size_t index)
{
    return LocalCoord {
        static_cast<LocalCoord::value_type>((index % CHUNK_SIZE)),
        static_cast<LocalCoord::value_type>((index / CHUNK_SIZE) / CHUNK_SIZE),
        static_cast<LocalCoord::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE),
    };
}

constexpr static inline VoxelCoord coord::to_voxel(const ChunkCoord &cv, const LocalCoord &lv)
{
    return VoxelCoord {
        static_cast<VoxelCoord::value_type>(lv.x + (cv.x << CHUNK_BIT_SHIFT)),
        static_cast<VoxelCoord::value_type>(lv.y + (cv.y << CHUNK_BIT_SHIFT)),
        static_cast<VoxelCoord::value_type>(lv.z + (cv.z << CHUNK_BIT_SHIFT)),
    };
}

constexpr static inline VoxelCoord coord::to_voxel(const glm::dvec3 &dv)
{
    return VoxelCoord {
        static_cast<VoxelCoord::value_type>(dv.x),
        static_cast<VoxelCoord::value_type>(dv.y),
        static_cast<VoxelCoord::value_type>(dv.z),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const ChunkCoord &cv)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(cv.x << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cv.y << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cv.z << CHUNK_BIT_SHIFT),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const VoxelCoord &vv)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(vv.x),
        static_cast<glm::dvec3::value_type>(vv.y),
        static_cast<glm::dvec3::value_type>(vv.z),
    };
}

constexpr static inline std::size_t coord::to_index(const LocalCoord &lv)
{
    return static_cast<std::size_t>((lv.y * CHUNK_SIZE + lv.z) * CHUNK_SIZE + lv.x);
}

#endif /* SHARED_COORD_HH */
