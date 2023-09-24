// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B7F3D211_9B56_4E57_B6AC_A6DC179EB279
#define B7F3D211_9B56_4E57_B6AC_A6DC179EB279
#include <client/glxx/object.hh>

namespace glxx
{
struct DrawArraysCmd final {
    int32_t vertices;
    int32_t instances;
    int32_t base_vertex;
    uint32_t base_instance;
};

struct DrawElementsCmd final {
    int32_t indices;
    int32_t instances;
    int32_t base_index;
    int32_t base_vertex;
    uint32_t base_instance;
};

class DrawCommand final {
public:
    DrawCommand() = default;
    DrawCommand(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance);
    DrawCommand(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance);
    DrawCommand(DrawCommand &&rhs);
    DrawCommand &operator=(DrawCommand &&rhs);
    constexpr bool valid() const;
    void set(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance);
    void set(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance);
    void invoke() const;
    size_t size() const;

private:
    bool indexed {false};
    uint32_t mode {0};
    uint32_t type {0};
    union {
        DrawArraysCmd a;
        DrawElementsCmd e;
    } cmd;
};
}

inline glxx::DrawCommand::DrawCommand(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance)
    : indexed{false}, mode{mode}, type{0}
{
    set(mode, vertices, instances, base_vertex, base_instance);
}

inline glxx::DrawCommand::DrawCommand(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance)
    : indexed{true}, mode{mode}, type{type}
{
    set(mode, type, indices, instances, base_index, base_vertex, base_instance);
}

inline glxx::DrawCommand::DrawCommand(glxx::DrawCommand &&rhs)
{
    indexed = rhs.indexed;
    mode = rhs.mode;
    type = rhs.type;
    cmd = rhs.cmd;
    rhs.indexed = false;
    rhs.mode = 0;
    rhs.type = 0;
}

inline glxx::DrawCommand &glxx::DrawCommand::operator=(glxx::DrawCommand &&rhs)
{
    glxx::DrawCommand copy(std::move(rhs));
    std::swap(indexed, rhs.indexed);
    std::swap(mode, rhs.mode);
    std::swap(type, rhs.type);
    std::swap(cmd, rhs.cmd);
    return *this;
}

inline constexpr bool glxx::DrawCommand::valid() const
{
    return mode != 0;
}

inline void glxx::DrawCommand::set(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance)
{
    indexed = false;
    this->mode = mode;
    cmd.a.vertices = static_cast<int32_t>(vertices);
    cmd.a.instances = static_cast<int32_t>(instances);
    cmd.a.base_vertex = static_cast<int32_t>(base_vertex);
    cmd.a.base_instance = static_cast<uint32_t>(base_instance);
}

inline void glxx::DrawCommand::set(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance)
{
    indexed = true;
    this->mode = mode;
    this->type = type;
    cmd.e.indices = static_cast<int32_t>(indices);
    cmd.e.instances = static_cast<int32_t>(instances);
    cmd.e.base_index = static_cast<int32_t>(base_index);
    cmd.e.base_vertex = static_cast<int32_t>(base_vertex);
    cmd.e.base_instance = static_cast<uint32_t>(base_instance);
}

inline void glxx::DrawCommand::invoke() const
{
    if(indexed) {
        glDrawElementsInstancedBaseVertexBaseInstance(
            mode,
            cmd.e.indices,
            type,
            reinterpret_cast<const void *>(cmd.e.base_index),
            cmd.e.instances,
            cmd.e.base_vertex,
            cmd.e.base_instance);
        return;
    }
    else {
        glDrawArraysInstancedBaseInstance(
            mode,
            cmd.a.base_vertex,
            cmd.a.vertices,
            cmd.a.instances,
            cmd.a.base_instance);
        return;
    }
}

inline size_t glxx::DrawCommand::size() const
{
    return indexed ? cmd.e.indices : cmd.a.vertices;
}

#endif /* B7F3D211_9B56_4E57_B6AC_A6DC179EB279 */
