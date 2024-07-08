#ifndef SHARED_COORD_HH
#define SHARED_COORD_HH
#include <functional>
#include <glm/common.hpp>
#include <shared/const.hh>

using ChunkPos = glm::vec<3, int32_t>;
using LocalPos = glm::vec<3, int16_t>;
using VoxelPos = glm::vec<3, int64_t>;

template<>
struct std::hash<ChunkPos> final {
    constexpr inline size_t operator()(const ChunkPos &cpos) const
    {
        size_t value = 0;
        value ^= cpos.x * 73856093;
        value ^= cpos.y * 19349663;
        value ^= cpos.z * 83492791;
        return value;
    }
};

template<>
struct std::hash<VoxelPos> final {
    constexpr inline size_t operator()(const VoxelPos &vpos) const
    {
        size_t value = 0;
        value ^= vpos.x * 73856093;
        value ^= vpos.y * 19349663;
        value ^= vpos.z * 83492791;
        return value;
    }
};

namespace coord
{
constexpr static ChunkPos to_chunk(const VoxelPos &vpos);
constexpr static LocalPos to_local(const VoxelPos &vpos);
constexpr static LocalPos to_local(const size_t index);
constexpr static VoxelPos to_voxel(const ChunkPos &cpos, const LocalPos &lpos);
constexpr static VoxelPos to_voxel(const glm::dvec3 &dvec);
constexpr static glm::dvec3 to_dvec(const ChunkPos &cpos);
constexpr static glm::dvec3 to_dvec(const VoxelPos &vpos);
constexpr static size_t to_index(const LocalPos &lpos);
} // namespace coord

constexpr static inline ChunkPos coord::to_chunk(const VoxelPos &vpos)
{
    return ChunkPos {
        static_cast<ChunkPos::value_type>(vpos.x >> CHUNK_BIT_SHIFT),
        static_cast<ChunkPos::value_type>(vpos.y >> CHUNK_BIT_SHIFT),
        static_cast<ChunkPos::value_type>(vpos.z >> CHUNK_BIT_SHIFT),
    };
}

constexpr static inline LocalPos coord::to_local(const VoxelPos &vpos)
{
    return LocalPos {
        glm::abs(static_cast<LocalPos::value_type>(vpos.x % CHUNK_SIZE)),
        glm::abs(static_cast<LocalPos::value_type>(vpos.y % CHUNK_SIZE)),
        glm::abs(static_cast<LocalPos::value_type>(vpos.z % CHUNK_SIZE)),
    };
}

constexpr static inline LocalPos coord::to_local(const size_t index)
{
    return LocalPos {
        static_cast<LocalPos::value_type>((index / CHUNK_AREA)),
        static_cast<LocalPos::value_type>((index % CHUNK_SIZE)),
        static_cast<LocalPos::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE),
    };
}

constexpr static inline VoxelPos coord::to_voxel(const ChunkPos &cpos, const LocalPos &lpos)
{
    return VoxelPos {
        static_cast<VoxelPos::value_type>(lpos.x + (cpos.x << CHUNK_BIT_SHIFT)),
        static_cast<VoxelPos::value_type>(lpos.y + (cpos.y << CHUNK_BIT_SHIFT)),
        static_cast<VoxelPos::value_type>(lpos.z + (cpos.z << CHUNK_BIT_SHIFT)),
    };
}

constexpr static inline VoxelPos coord::to_voxel(const glm::dvec3 &dvec)
{
    return VoxelPos {
        static_cast<VoxelPos::value_type>(dvec.x),
        static_cast<VoxelPos::value_type>(dvec.y),
        static_cast<VoxelPos::value_type>(dvec.z),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const ChunkPos &cpos)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(cpos.x << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cpos.y << CHUNK_BIT_SHIFT),
        static_cast<glm::dvec3::value_type>(cpos.z << CHUNK_BIT_SHIFT),
    };
}

constexpr static inline glm::dvec3 coord::to_dvec(const VoxelPos &vpos)
{
    return glm::dvec3 {
        static_cast<glm::dvec3::value_type>(vpos.x),
        static_cast<glm::dvec3::value_type>(vpos.y),
        static_cast<glm::dvec3::value_type>(vpos.z),
    };
}

constexpr static inline size_t coord::to_index(const LocalPos &lpos)
{
    return static_cast<size_t>((lpos.x * CHUNK_SIZE + lpos.z) * CHUNK_SIZE + lpos.y);
}

#endif /* SHARED_COORD_HH */
