// SPDX-License-Identifier: Zlib
#ifndef SHARED_EVENT_CHUNK_CREATE_HH
#define SHARED_EVENT_CHUNK_CREATE_HH
#include <shared/chunk.hh>

struct ChunkCreateEvent final {
    Chunk *chunk {nullptr};
    ChunkPos cpos {};
};

#endif /* SHARED_EVENT_CHUNK_CREATE_HH */
