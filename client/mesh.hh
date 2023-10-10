// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_MESH_HH
#define CLIENT_MESH_HH
#include <client/glxx/buffer.hh>
#include <vector>

struct Mesh final {
    size_t vertices {};
    size_t triangles {};
    glxx::Buffer vbo {};
};

template<typename vert>
class MeshBuilder final {
public:
    void clear();
    void push_back(const vert &vtx);
    void push_back(const vert &a, const vert &b, const vert &c);
    void push_back(const vert &a, const vert &b, const vert &c, const vert &d);

    const bool empty() const;
    const size_t size() const;
    const size_t bytes() const;
    const vert *data() const;

private:
    std::vector<vert> vec {};
};

template<typename vert>
inline void MeshBuilder<vert>::clear()
{
    vec.clear();
}

template<typename vert>
inline void MeshBuilder<vert>::push_back(const vert &vtx)
{
    vec.push_back(vtx);
}

template<typename vert>
inline void MeshBuilder<vert>::push_back(const vert &a, const vert &b, const vert &c)
{
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);
}

template<typename vert>
inline void MeshBuilder<vert>::push_back(const vert &a, const vert &b, const vert &c, const vert &d)
{
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);

    vec.push_back(c);
    vec.push_back(d);
    vec.push_back(a);
}

template<typename vert>
inline const bool MeshBuilder<vert>::empty() const
{
    return vec.empty();
}

template<typename vert>
inline const size_t MeshBuilder<vert>::size() const
{
    return vec.size();
}

template<typename vert>
inline const size_t MeshBuilder<vert>::bytes() const
{
    return vec.size() * sizeof(vert);
}

template<typename vert>
inline const vert *MeshBuilder<vert>::data() const
{
    return vec.data();
}

#endif/* CLIENT_MESH_HH */
