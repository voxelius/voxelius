// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_RENDER_GL_VERTEXARRAY_HH
#define CLIENT_RENDER_GL_VERTEXARRAY_HH
#include <client/render/gl/buffer.hh>
#include <spdlog/spdlog.h>

namespace gl
{
class VertexArray final : public gl::Object<VertexArray> {
public:
    VertexArray() = default;
    VertexArray(VertexArray &&rhs);
    VertexArray &operator=(VertexArray &&rhs);
    void create();
    void destroy();
    void bind() const;
    void set_index_buffer(const gl::Buffer &ibo);
    void set_vertex_buffer(uint32_t binding, const gl::Buffer &vbo, size_t stride);
    void enable_attribute(uint32_t attrib, bool enable);
    void set_attribute_format(uint32_t attrib, uint32_t type, size_t count, size_t offset, bool normalized);
    void set_attribute_binding(uint32_t attrib, uint32_t binding);
    static void unbind();
};
} // namespace gl

inline gl::VertexArray::VertexArray(gl::VertexArray &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline gl::VertexArray &gl::VertexArray::operator=(gl::VertexArray &&rhs)
{
    gl::VertexArray copy(std::move(rhs));
    std::swap(handle, copy.handle);
    return *this;
}

inline void gl::VertexArray::create()
{
    destroy();
    glCreateVertexArrays(1, &handle);
}

inline void gl::VertexArray::destroy()
{
    if(handle) {
        glDeleteVertexArrays(1, &handle);
        handle = 0;
    }
}

inline void gl::VertexArray::bind() const
{
    glBindVertexArray(handle);
}

inline void gl::VertexArray::set_index_buffer(const gl::Buffer &ibo)
{
    glVertexArrayElementBuffer(handle, ibo.get());
}

inline void gl::VertexArray::set_vertex_buffer(uint32_t binding, const gl::Buffer &vbo, size_t stride)
{
    glVertexArrayVertexBuffer(handle, binding, vbo.get(), 0, static_cast<int32_t>(stride));
}

inline void gl::VertexArray::enable_attribute(uint32_t attrib, bool enable)
{
    if(enable) {
        glEnableVertexArrayAttrib(handle, attrib);
        return;
    }

    glDisableVertexArrayAttrib(handle, attrib);
}

inline void gl::VertexArray::set_attribute_format(uint32_t attrib, uint32_t type, size_t count, size_t offset, bool normalized)
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

inline void gl::VertexArray::set_attribute_binding(uint32_t attrib, uint32_t binding)
{
    glVertexArrayAttribBinding(handle, attrib, binding);
}

inline void gl::VertexArray::unbind()
{
    glBindVertexArray(0);
}

#endif/* CLIENT_RENDER_GL_VERTEXARRAY_HH */
