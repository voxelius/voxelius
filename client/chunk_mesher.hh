// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CHUNK_MESHER_HH
#define CLIENT_CHUNK_MESHER_HH
#include <client/chunk_vbo.hh>
#include <vector>

struct ChunkMeshComponent final {
    std::vector<ChunkVBO> quad {};
};

namespace chunk_mesher
{
void init(void);
void deinit(void);
void update(void);
} // namespace chunk_mesher

#endif /* CLIENT_CHUNK_MESHER_HH */
