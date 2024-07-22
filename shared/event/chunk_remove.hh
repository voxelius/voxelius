#ifndef SHARED_EVENT_CHUNK_REMOVE_HH
#define SHARED_EVENT_CHUNK_REMOVE_HH
#include <shared/chunk.hh>

struct ChunkRemoveEvent final {
    ChunkPos coord {};
    Chunk *chunk {};
};

#endif /* SHARED_EVENT_CHUNK_REMOVE_HH */
