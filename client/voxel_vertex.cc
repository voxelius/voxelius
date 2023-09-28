// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/glxx/vertex_array.hh>
#include <client/shaders.hh>
#include <client/voxel_vertex.hh>
#include <shared/cxmath.hh>

constexpr static const char *voxel_vertex_vert = R"glsl(
    layout(location = 0) in uint _voxel_vertex_data_x;
    layout(location = 1) in uint _voxel_vertex_data_y;
    layout(location = 2) in vec3 _voxel_vertex_data_norm;
    layout(location = 3) in vec2 _voxel_vertex_data_uv;
    #define voxel_vertex_norm() _voxel_vertex_data_norm
    #define voxel_vertex_uv() _voxel_vertex_data_uv
    vec3 voxel_vertex_position()
    {
        vec3 position;
        position.x = float(uint(_voxel_vertex_data_x >> 22U) & 0x3FFU) / 512.0 * 32.0;
        position.y = float(uint(_voxel_vertex_data_x >> 12U) & 0x3FFU) / 512.0 * 32.0;
        position.z = float(uint(_voxel_vertex_data_x >> 2U) & 0x3FFU) / 512.0 * 32.0;
        return position;
    }
    uint voxel_vertex_toffset()
    {
        return ((_voxel_vertex_data_y & 0xFFFF0000U) >> 16U);
    }
    uint voxel_vertex_tsize()
    {
        return (_voxel_vertex_data_y & 0x0000FFFFU);
    }
)glsl";

VoxelVertex::VoxelVertex(const vec3f_t &position, const vec3f_t &norm, const vec2f_t &uv, uint16_t toffset, uint16_t tsize)
{
    data_x |= (static_cast<uint32_t>(position.x / 32.0f * 512.0f) & 0x3FF) << 22;
    data_x |= (static_cast<uint32_t>(position.y / 32.0f * 512.0f) & 0x3FF) << 12;
    data_x |= (static_cast<uint32_t>(position.z / 32.0f * 512.0f) & 0x3FF) << 2;
    data_y |= (static_cast<uint32_t>(toffset) << 16);
    data_y |= (static_cast<uint32_t>(tsize) << 0);
    data_norm = norm;
    data_uv = uv;
}

void VoxelVertex::setup(glxx::VertexArray &vao)
{
    vao.enable_attribute(0, true);
    vao.enable_attribute(1, true);
    vao.enable_attribute(2, true);
    vao.enable_attribute(3, true);

    vao.set_attribute_format(0, GL_UNSIGNED_INT, 1, offsetof(VoxelVertex, data_x), false);
    vao.set_attribute_format(1, GL_UNSIGNED_INT, 1, offsetof(VoxelVertex, data_y), false);
    vao.set_attribute_format(2, GL_FLOAT, 3, offsetof(VoxelVertex, data_norm), false);
    vao.set_attribute_format(3, GL_FLOAT, 2, offsetof(VoxelVertex, data_uv), false);

    vao.set_attribute_binding(0, VOXEL_VERTEX_VBO_BINDING);
    vao.set_attribute_binding(1, VOXEL_VERTEX_VBO_BINDING);
    vao.set_attribute_binding(2, VOXEL_VERTEX_VBO_BINDING);
    vao.set_attribute_binding(3, VOXEL_VERTEX_VBO_BINDING);
}

void VoxelVertex::init()
{
    shaders::add("voxel_vertex", GL_VERTEX_SHADER, voxel_vertex_vert);
}
