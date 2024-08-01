// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_WORLD_CHUNK_MESHER_HH
#define CLIENT_WORLD_CHUNK_MESHER_HH
#include <cstddef>
#include <glad/gl.h>
#include <vector>

class ChunkVBO final {
public:
    std::size_t size {};
    GLuint handle {};

public:
    inline ~ChunkVBO(void)
    {
        // A ChunkVBO object is supposed to be a part of
        // the ChunkMesh component; when the EnTT registry
        // is cleared or a chunk is nuked, all the vertex
        // buffers within the mesh components are expected
        // to be disposed without leaving zombie OpenGL handles
        if(handle) glDeleteBuffers(1, &handle);
    }
};

struct ChunkMeshComponent final {
    std::vector<ChunkVBO> quad {};
};

namespace chunk_mesher
{
void init(void);
void deinit(void);
void update(void);
} // namespace chunk_mesher

#endif /* CLIENT_WORLD_CHUNK_MESHER_HH */
