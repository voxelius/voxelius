// SPDX-License-Identifier: Zlib
#ifndef SHARED_CHUNK_HH
#define SHARED_CHUNK_HH
#include <cstdint>
#include <entt/entity/entity.hpp>
#include <shared/coord.hh>
#include <vector>

enum class ChunkType {
    UNALLOCATED,    // Chunk has no voxels
    SINGLE_VOXEL,   // The entire chunk is just one voxel
    PALETTE_UINT8,  // The chunk uses an 8-bit swatch and a palette
    PALETTE_UINT16, // The chunk uses a 16-bit swatch and a palette
};

struct Chunk final {
    entt::entity entity {};
    ChunkType type {};
    void *voxels {};
};

struct ChunkComponent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_CHUNK_HH */
