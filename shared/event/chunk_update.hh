// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_EVENT_CHUNK_UPDATE_HH
#define SHARED_EVENT_CHUNK_UPDATE_HH
#include <shared/chunk.hh>

struct ChunkUpdateEvent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_EVENT_CHUNK_UPDATE_HH */
