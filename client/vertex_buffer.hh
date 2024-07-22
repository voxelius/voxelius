// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_VERTEX_BUFFER_HH
#define CLIENT_VERTEX_BUFFER_HH
#include <cstddef>
#include <glad/gl.h>

class VertexBuffer final {
public:
    std::size_t size {};
    GLuint handle {};

public:
    inline ~VertexBuffer(void)
    {
        // VertexBuffer objects are parts of some entity
        // components; the entity system expects GL objects
        // to be safely destroyed so a destructor is needed
        if(handle) glDeleteBuffers(1, &handle);
    }
};


#endif /* CLIENT_VERTEX_BUFFER_HH */
