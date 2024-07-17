// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_EVENT_CHUNK_REMOVE_HH
#define SHARED_EVENT_CHUNK_REMOVE_HH
#include <shared/chunk.hh>

struct ChunkRemoveEvent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_EVENT_CHUNK_REMOVE_HH */
