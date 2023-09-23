// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef E69DB236_81BC_4E98_B5E0_FAE6650B602F
#define E69DB236_81BC_4E98_B5E0_FAE6650B602F
#include <client/glxx/object.hh>

namespace glxx
{
class Buffer final : public Object<Buffer> {
public:
    Buffer() = default;
    Buffer(Buffer &&rhs);
    Buffer &operator=(Buffer &&rhs);
    void create();
    void destroy();
    void bind_base(GLenum target, uint32_t index);
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

inline void glxx::Buffer::bind_base(GLenum target, uint32_t index)
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

#endif /* E69DB236_81BC_4E98_B5E0_FAE6650B602F */
