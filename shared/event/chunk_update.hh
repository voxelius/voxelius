// SPDX-License-Identifier: Zlib
#ifndef SHARED_EVENT_CHUNK_UPDATE_HH
#define SHARED_EVENT_CHUNK_UPDATE_HH
#include <shared/chunk.hh>

struct ChunkUpdateEvent final {
    Chunk *chunk {nullptr};
    Voxel voxel {NULL_VOXEL};
    ChunkPos cpos {};
    LocalPos lpos {};
    VoxelPos vpos {};
    size_t index {};
};

#endif /* SHARED_EVENT_CHUNK_UPDATE_HH */
