// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLXX_VERTEX_ARRAY_HH
#define CLIENT_GLXX_VERTEX_ARRAY_HH
#include <client/glxx/buffer.hh>
#include <spdlog/spdlog.h>

namespace glxx
{
class VertexArray final : public Object<VertexArray> {
public:
    VertexArray() = default;
    VertexArray(VertexArray &&rhs);
    VertexArray &operator=(VertexArray &&rhs);
    void create();
    void destroy();
    void bind() const;
    void set_index_buffer(const Buffer &ibo);
    void set_vertex_buffer(uint32_t binding, const Buffer &vbo, size_t stride);
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

#endif /* CLIENT_GLXX_VERTEX_ARRAY_HH */
