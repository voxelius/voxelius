// SPDX-License-Identifier: Zlib
#ifndef SHARED_EVENT_CHUNK_REMOVE_HH
#define SHARED_EVENT_CHUNK_REMOVE_HH
#include <shared/chunk.hh>

struct ChunkRemoveEvent final {
    Chunk *chunk {nullptr};
    ChunkPos cpos {};
};

#endif /* SHARED_EVENT_CHUNK_REMOVE_HH */
