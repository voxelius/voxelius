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
#ifndef CLIENT_GLXX_BUFFER_HH
#define CLIENT_GLXX_BUFFER_HH
#include <client/glxx/object.hh>

namespace glxx
{
class Buffer final : public glxx::Object<Buffer> {
public:
    Buffer() = default;
    Buffer(Buffer &&rhs);
    Buffer &operator=(Buffer &&rhs);
    void create();
    void destroy();
    void bind_base(uint32_t target, uint32_t index);
    void storage(size_t size, const void *data, GLbitfield flags);
    void resize(size_t new_size, const void *data, GLbitfield usage);
    void write(size_t offset, size_t size, const void *data);
    void read(size_t offset, size_t size, void *data) const;
};
} // namespace glxx

inline glxx::Buffer::Buffer(glxx::Buffer &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::Buffer &glxx::Buffer::operator=(glxx::Buffer &&rhs)
{
    glxx::Buffer copy {std::move(rhs)};
    std::swap(handle, copy.handle);
    return *this;
}

inline void glxx::Buffer::create()
{
    destroy();
    glCreateBuffers(1, &handle);
}

inline void glxx::Buffer::destroy()
{
    if(handle) {
        glDeleteBuffers(1, &handle);
        handle = 0;
    }
}

inline void glxx::Buffer::bind_base(uint32_t target, uint32_t index)
{
    glBindBufferBase(target, index, handle);
}

inline void glxx::Buffer::storage(size_t size, const void *data, GLbitfield flags)
{
    glNamedBufferStorage(handle, static_cast<GLsizeiptr>(size), data, flags);
}

inline void glxx::Buffer::resize(size_t new_size, const void *data, GLbitfield usage)
{
    glNamedBufferData(handle, static_cast<GLsizeiptr>(new_size), data, usage);
}

inline void glxx::Buffer::write(size_t offset, size_t size, const void *data)
{
    glNamedBufferSubData(handle, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}

inline void glxx::Buffer::read(size_t offset, size_t size, void *data) const
{
    glGetNamedBufferSubData(handle, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}

#endif /* CLIENT_GLXX_BUFFER_HH */
