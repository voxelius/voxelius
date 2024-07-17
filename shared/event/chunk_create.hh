// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_EVENT_CHUNK_CREATE_HH
#define SHARED_EVENT_CHUNK_CREATE_HH
#include <shared/chunk.hh>

struct ChunkCreateEvent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_EVENT_CHUNK_CREATE_HH */
