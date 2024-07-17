// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#ifndef SHARED_EVENT_CHUNK_FILL_HH
#define SHARED_EVENT_CHUNK_FILL_HH
#include <shared/chunk.hh>
#include <shared/voxel.hh>

struct ChunkFillEvent final {
    ChunkCoord coord {};
    Chunk *chunk {};
    Voxel voxel {};
};

#endif /* SHARED_EVENT_CHUNK_FILL_HH */
