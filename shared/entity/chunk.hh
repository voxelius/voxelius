// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_CHUNK_HH
#define SHARED_ENTITY_CHUNK_HH
#include <shared/chunk.hh>

struct ChunkComponent final {
    ChunkCoord coord {};
    Chunk *chunk {};
};

#endif /* SHARED_ENTITY_CHUNK_HH */
