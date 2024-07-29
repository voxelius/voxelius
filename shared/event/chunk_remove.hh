#ifndef SHARED_EVENT_CHUNK_REMOVE_HH
#define SHARED_EVENT_CHUNK_REMOVE_HH
#include <shared/world/chunk.hh>

struct ChunkRemoveEvent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_EVENT_CHUNK_REMOVE_HH */
