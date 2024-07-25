// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_COORD_HH
#define SHARED_COORD_HH
#include <cstdint>
#include <functional>
#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <shared/const.hh>

using ChunkPos = glm::vec<3, std::int32_t>;
using LocalPos = glm::vec<3, std::int16_t>;
using VoxelPos = glm::vec<3, std::int64_t>;

struct EntityPos final {
    ChunkPos chunk {};
    glm::fvec3 local {};
};

template<>
struct std::hash<ChunkPos> final {
    constexpr inline std::size_t operator()(const ChunkPos &cpos) const
    {
        std::size_t value = 0;
        value ^= cpos.x * 73856093;
        value ^= cpos.y * 19349663;
        value ^= cpos.z * 83492791;
        return value;
    }
};

template<>
struct std::hash<VoxelPos> final {
    constexpr inline std::size_t operator()(const VoxelPos &vpos) const
    {
        std::size_t value = 0;
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
constexpr static LocalPos to_local(const EntityPos &epos);
constexpr static LocalPos to_local(std::size_t index);
constexpr static VoxelPos to_voxel(const ChunkPos &cpos);
constexpr static VoxelPos to_voxel(const ChunkPos &cpos, const LocalPos &lpos);
constexpr static VoxelPos to_voxel(const EntityPos &epos);
constexpr static glm::fvec3 to_world(const ChunkPos &cpos);
constexpr static glm::fvec3 to_world(const LocalPos &lpos);
constexpr static glm::fvec3 to_world(const VoxelPos &vpos);
constexpr static glm::fvec3 to_world(const EntityPos &epos);
constexpr static glm::fvec3 to_world(const EntityPos &pivot, const ChunkPos &cpos);
constexpr static glm::fvec3 to_world(const EntityPos &pivot, const EntityPos &epos);
constexpr static std::size_t to_index(const LocalPos &lpos);
constexpr static EntityPos to_entity(const VoxelPos &vpos);
} // namespace coord

constexpr static inline ChunkPos coord::to_chunk(const VoxelPos &vpos)
{
    ChunkPos result = {};
    result.x = static_cast<ChunkPos::value_type>(vpos.x >> CHUNK_SIZE_LOG2);
    result.y = static_cast<ChunkPos::value_type>(vpos.y >> CHUNK_SIZE_LOG2);
    result.z = static_cast<ChunkPos::value_type>(vpos.z >> CHUNK_SIZE_LOG2);
    return result;
}

constexpr static inline LocalPos coord::to_local(const VoxelPos &vpos)
{
    LocalPos result = {};
    result.x = glm::abs(static_cast<LocalPos::value_type>(vpos.x % CHUNK_SIZE));
    result.y = glm::abs(static_cast<LocalPos::value_type>(vpos.y % CHUNK_SIZE));
    result.z = glm::abs(static_cast<LocalPos::value_type>(vpos.z % CHUNK_SIZE));
    return result;
}

constexpr static inline LocalPos coord::to_local(const EntityPos &epos)
{
    LocalPos result = {};
    result.x = static_cast<LocalPos::value_type>(epos.local.x);
    result.y = static_cast<LocalPos::value_type>(epos.local.y);
    result.z = static_cast<LocalPos::value_type>(epos.local.z);
    return result;
}

constexpr static inline LocalPos coord::to_local(std::size_t index)
{
    LocalPos result = {};
    result.x = static_cast<LocalPos::value_type>((index % CHUNK_SIZE));
    result.y = static_cast<LocalPos::value_type>((index / CHUNK_SIZE) / CHUNK_SIZE);
    result.z = static_cast<LocalPos::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE);
    return result;
}

constexpr static inline VoxelPos coord::to_voxel(const ChunkPos &cpos)
{
    VoxelPos result = {};
    result.x = static_cast<VoxelPos::value_type>(cpos.x << CHUNK_SIZE_LOG2);
    result.y = static_cast<VoxelPos::value_type>(cpos.y << CHUNK_SIZE_LOG2);
    result.z = static_cast<VoxelPos::value_type>(cpos.z << CHUNK_SIZE_LOG2);
    return result;
}

constexpr static inline VoxelPos coord::to_voxel(const ChunkPos &cpos, const LocalPos &lpos)
{
    VoxelPos result = {};
    result.x = static_cast<VoxelPos::value_type>(cpos.x << CHUNK_SIZE_LOG2) + lpos.x;
    result.y = static_cast<VoxelPos::value_type>(cpos.y << CHUNK_SIZE_LOG2) + lpos.y;
    result.z = static_cast<VoxelPos::value_type>(cpos.z << CHUNK_SIZE_LOG2) + lpos.z;
    return result;
}

constexpr static inline VoxelPos coord::to_voxel(const EntityPos &epos)
{
    VoxelPos result = {};
    result.x = static_cast<VoxelPos::value_type>((epos.chunk.x << CHUNK_SIZE_LOG2) + epos.local.x);
    result.y = static_cast<VoxelPos::value_type>((epos.chunk.y << CHUNK_SIZE_LOG2) + epos.local.y);
    result.z = static_cast<VoxelPos::value_type>((epos.chunk.z << CHUNK_SIZE_LOG2) + epos.local.z);
    return result;
}

constexpr static inline glm::fvec3 coord::to_world(const ChunkPos &cpos)
{
    glm::fvec3 result = {};
    result.x = static_cast<glm::fvec3::value_type>(cpos.x << CHUNK_SIZE_LOG2);
    result.y = static_cast<glm::fvec3::value_type>(cpos.y << CHUNK_SIZE_LOG2);
    result.z = static_cast<glm::fvec3::value_type>(cpos.z << CHUNK_SIZE_LOG2);
    return result;
}

constexpr static inline glm::fvec3 coord::to_world(const LocalPos &lpos)
{
    glm::fvec3 result = {};
    result.x = static_cast<glm::fvec3::value_type>(lpos.x);
    result.y = static_cast<glm::fvec3::value_type>(lpos.y);
    result.z = static_cast<glm::fvec3::value_type>(lpos.z);
    return result;
}

constexpr static inline glm::fvec3 coord::to_world(const VoxelPos &vpos)
{
    glm::fvec3 result = {};
    result.x = static_cast<glm::fvec3::value_type>(vpos.x);
    result.y = static_cast<glm::fvec3::value_type>(vpos.y);
    result.z = static_cast<glm::fvec3::value_type>(vpos.z);
    return result;
}

constexpr static inline glm::fvec3 coord::to_world(const EntityPos &epos)
{
    glm::fvec3 result = {};
    result.x = static_cast<glm::fvec3::value_type>(epos.local.x + (epos.chunk.x << CHUNK_SIZE_LOG2));
    result.y = static_cast<glm::fvec3::value_type>(epos.local.y + (epos.chunk.y << CHUNK_SIZE_LOG2));
    result.z = static_cast<glm::fvec3::value_type>(epos.local.z + (epos.chunk.z << CHUNK_SIZE_LOG2));
    return result;
}

constexpr static inline glm::fvec3 coord::to_world(const EntityPos &pivot, const ChunkPos &cpos)
{
    glm::fvec3 result = {};
    result.x = static_cast<glm::fvec3::value_type>((cpos.x - pivot.chunk.x) << CHUNK_SIZE_LOG2) - pivot.local.x;
    result.y = static_cast<glm::fvec3::value_type>((cpos.y - pivot.chunk.y) << CHUNK_SIZE_LOG2) - pivot.local.y;
    result.z = static_cast<glm::fvec3::value_type>((cpos.z - pivot.chunk.z) << CHUNK_SIZE_LOG2) - pivot.local.z;
    return result;
}

constexpr static inline glm::fvec3 coord::to_world(const EntityPos &pivot, const EntityPos &epos)
{
    glm::fvec3 result = {};
    result.x = static_cast<glm::fvec3::value_type>((epos.chunk.x - pivot.chunk.x) << CHUNK_SIZE_LOG2) + (epos.local.x - pivot.local.x);
    result.y = static_cast<glm::fvec3::value_type>((epos.chunk.y - pivot.chunk.y) << CHUNK_SIZE_LOG2) + (epos.local.y - pivot.local.y);
    result.z = static_cast<glm::fvec3::value_type>((epos.chunk.z - pivot.chunk.z) << CHUNK_SIZE_LOG2) + (epos.local.z - pivot.local.z);
    return result;
}

constexpr static inline std::size_t coord::to_index(const LocalPos &lpos)
{
    return static_cast<std::size_t>((lpos.y * CHUNK_SIZE + lpos.z) * CHUNK_SIZE + lpos.x);
}

constexpr static inline EntityPos coord::to_entity(const VoxelPos &vpos)
{
    EntityPos result = {};
    result.chunk.x = static_cast<ChunkPos::value_type>(vpos.x >> CHUNK_SIZE_LOG2);
    result.chunk.y = static_cast<ChunkPos::value_type>(vpos.y >> CHUNK_SIZE_LOG2);
    result.chunk.z =static_cast<ChunkPos::value_type>(vpos.z >> CHUNK_SIZE_LOG2);
    result.local.x = glm::abs(static_cast<glm::fvec3::value_type>(vpos.x % CHUNK_SIZE));
    result.local.y = glm::abs(static_cast<glm::fvec3::value_type>(vpos.y % CHUNK_SIZE));
    result.local.z = glm::abs(static_cast<glm::fvec3::value_type>(vpos.z % CHUNK_SIZE));
    return result;
}

#endif /* SHARED_COORD_HH */
