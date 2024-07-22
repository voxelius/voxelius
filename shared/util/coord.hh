// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_UTIL_COORD_HH
#define SHARED_UTIL_COORD_HH
#include <glm/common.hpp>
#include <shared/const.hh>
#include <shared/types.hh>

namespace util
{
constexpr static ChunkPos to_chunk(const VoxelPos &vpos);
constexpr static LocalPos to_local(const VoxelPos &vpos);
constexpr static LocalPos to_local(const std::size_t index);
constexpr static VoxelPos to_voxel(const ChunkPos &cpos, const LocalPos &lpos);
constexpr static VoxelPos to_voxel(const glm::fvec3 &fvec);
constexpr static glm::fvec3 to_world(const ChunkPos &cpos);
constexpr static glm::fvec3 to_world(const VoxelPos &vpos);
constexpr static std::size_t to_index(const LocalPos &lpos);
} // namespace util

constexpr static inline ChunkPos util::to_chunk(const VoxelPos &vpos)
{
    return ChunkPos {
        static_cast<ChunkPos::value_type>(vpos.x >> CHUNK_SIZE_LOG2),
        static_cast<ChunkPos::value_type>(vpos.y >> CHUNK_SIZE_LOG2),
        static_cast<ChunkPos::value_type>(vpos.z >> CHUNK_SIZE_LOG2),
    };
}

constexpr static inline LocalPos util::to_local(const VoxelPos &vpos)
{
    return LocalPos {
        glm::abs(static_cast<LocalPos::value_type>(vpos.x % CHUNK_SIZE)),
        glm::abs(static_cast<LocalPos::value_type>(vpos.y % CHUNK_SIZE)),
        glm::abs(static_cast<LocalPos::value_type>(vpos.z % CHUNK_SIZE)),
    };
}

constexpr static inline LocalPos util::to_local(const std::size_t index)
{
    return LocalPos {
        static_cast<LocalPos::value_type>((index % CHUNK_SIZE)),
        static_cast<LocalPos::value_type>((index / CHUNK_SIZE) / CHUNK_SIZE),
        static_cast<LocalPos::value_type>((index / CHUNK_SIZE) % CHUNK_SIZE),
    };
}

constexpr static inline VoxelPos util::to_voxel(const ChunkPos &cpos, const LocalPos &lpos)
{
    return VoxelPos {
        static_cast<VoxelPos::value_type>(lpos.x + (cpos.x << CHUNK_SIZE_LOG2)),
        static_cast<VoxelPos::value_type>(lpos.y + (cpos.y << CHUNK_SIZE_LOG2)),
        static_cast<VoxelPos::value_type>(lpos.z + (cpos.z << CHUNK_SIZE_LOG2)),
    };
}

constexpr static inline VoxelPos util::to_voxel(const glm::fvec3 &fvec)
{
    return VoxelPos {
        static_cast<VoxelPos::value_type>(fvec.x),
        static_cast<VoxelPos::value_type>(fvec.y),
        static_cast<VoxelPos::value_type>(fvec.z),
    };
}

constexpr static inline glm::fvec3 util::to_world(const ChunkPos &cpos)
{
    return glm::fvec3 {
        static_cast<glm::fvec3::value_type>(cpos.x << CHUNK_SIZE_LOG2),
        static_cast<glm::fvec3::value_type>(cpos.y << CHUNK_SIZE_LOG2),
        static_cast<glm::fvec3::value_type>(cpos.z << CHUNK_SIZE_LOG2),
    };
}

constexpr static inline glm::fvec3 util::to_world(const VoxelPos &vpos)
{
    return glm::fvec3 {
        static_cast<glm::fvec3::value_type>(vpos.x),
        static_cast<glm::fvec3::value_type>(vpos.y),
        static_cast<glm::fvec3::value_type>(vpos.z),
    };
}

constexpr static inline std::size_t util::to_index(const LocalPos &lpos)
{
    return static_cast<std::size_t>((lpos.y * CHUNK_SIZE + lpos.z) * CHUNK_SIZE + lpos.x);
}

#endif /* SHARED_UTIL_COORD_HH */
