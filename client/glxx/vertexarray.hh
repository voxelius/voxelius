// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef CLIENT_GLXX_VERTEXARRAY_HH
#define CLIENT_GLXX_VERTEXARRAY_HH
#include <client/glxx/buffer.hh>
#include <spdlog/spdlog.h>

namespace glxx
{
class VertexArray final : public glxx::Object<VertexArray> {
public:
    VertexArray() = default;
    VertexArray(VertexArray &&rhs);
    VertexArray &operator=(VertexArray &&rhs);
    void create();
    void destroy();
    void bind() const;
    void set_index_buffer(const glxx::Buffer &ibo);
    void set_vertex_buffer(uint32_t binding, const glxx::Buffer &vbo, size_t stride);
    void enable_attribute(uint32_t attrib, bool enable);
    void set_attribute_format(uint32_t attrib, uint32_t type, size_t count, size_t offset, bool normalized);
    void set_attribute_binding(uint32_t attrib, uint32_t binding);
    static void unbind();
};
} // namespace glxx

inline glxx::VertexArray::VertexArray(glxx::VertexArray &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::VertexArray &glxx::VertexArray::operator=(glxx::VertexArray &&rhs)
{
    glxx::VertexArray copy(std::move(rhs));
    std::swap(handle, copy.handle);
    return *this;
}

inline void glxx::VertexArray::create()
{
    destroy();
    glCreateVertexArrays(1, &handle);
}

inline void glxx::VertexArray::destroy()
{
    if(handle) {
        glDeleteVertexArrays(1, &handle);
        handle = 0;
    }
}

inline void glxx::VertexArray::bind() const
{
    glBindVertexArray(handle);
}

inline void glxx::VertexArray::set_index_buffer(const glxx::Buffer &ibo)
{
    glVertexArrayElementBuffer(handle, ibo.get());
}

inline void glxx::VertexArray::set_vertex_buffer(uint32_t binding, const glxx::Buffer &vbo, size_t stride)
{
    glVertexArrayVertexBuffer(handle, binding, vbo.get(), 0, static_cast<int32_t>(stride));
}

inline void glxx::VertexArray::enable_attribute(uint32_t attrib, bool enable)
{
    if(enable) {
        glEnableVertexArrayAttrib(handle, attrib);
        return;
    }

    glDisableVertexArrayAttrib(handle, attrib);
}

inline void glxx::VertexArray::set_attribute_format(uint32_t attrib, uint32_t type, size_t count, size_t offset, bool normalized)
{
    switch(type) {
        case GL_FLOAT:
            glVertexArrayAttribFormat(handle, attrib, static_cast<int32_t>(count), type, normalized ? GL_TRUE : GL_FALSE, static_cast<uint32_t>(offset));
            break;
        case GL_INT:
        case GL_UNSIGNED_INT:
            glVertexArrayAttribIFormat(handle, attrib, static_cast<int32_t>(count), type, static_cast<uint32_t>(offset));
            break;
        default:
            spdlog::warn("GL: unknown attrib type {}", type);
            break;
    }
}

inline void glxx::VertexArray::set_attribute_binding(uint32_t attrib, uint32_t binding)
{
    glVertexArrayAttribBinding(handle, attrib, binding);
}

inline void glxx::VertexArray::unbind()
{
    glBindVertexArray(0);
}

#endif /* CLIENT_GLXX_VERTEXARRAY_HH */
