// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_ENTITY_CHUNK_MESH_HH
#define CLIENT_ENTITY_CHUNK_MESH_HH
#include <client/vertex_buffer.hh>
#include <vector>

struct ChunkMeshComponent final {
    std::vector<VertexBuffer> quad {};
};

#endif /* CLIENT_ENTITY_CHUNK_MESH_HH */
