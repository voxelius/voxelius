// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_CHUNK_VBO_HH
#define CLIENT_CHUNK_VBO_HH
#include <cstddef>
#include <glad/gl.h>

class ChunkVBO final {
public:
    std::size_t size {};
    GLuint handle {};

public:
    inline ~ChunkVBO(void)
    {
        // The ChunkVBO structure is meant to be a part
        // of the ChunkMesh component within the EnTT registry;
        // When the registry is cleaned or a chunk is removed, components
        // are expected to be safely disposed of so we need a destructor;
        if(handle) glDeleteBuffers(1, &handle);
    }
};

#endif /* CLIENT_CHUNK_VBO_HH */
