// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef SHARED_ENTITY_CHUNK_HH
#define SHARED_ENTITY_CHUNK_HH
#include <shared/chunk_pos.hh>
#include <shared/chunk.hh>

struct ChunkComponent final {
    ChunkPos coord {};
    Chunk *chunk {};
};

#endif /* SHARED_ENTITY_CHUNK_HH */
