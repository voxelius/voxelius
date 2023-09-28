// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/glxx/vertex_array.hh>
#include <client/shaders.hh>
#include <client/voxel_vertex.hh>
#include <shared/cxmath.hh>

constexpr static const char *voxel_vertex_vert = R"glsl(
    layout(location = 0) in uvec4 _vvdat_i;

    // vvdat_i[0]: 3x10-bit vertex position
    vec3 voxel_vertex_position()
    {
        vec3 position;
        position.x = float((_vvdat_i.x >> 20U) & 0x3FFU) / 511.0 * 16.0;
        position.y = float((_vvdat_i.x >> 10U) & 0x3FFU) / 511.0 * 16.0;
        position.z = float((_vvdat_i.x >>  0U) & 0x3FFU) / 511.0 * 16.0;
        return position;
    }

    // vvdat_i[1]: 3x10-bit vertex normal
    vec3 voxel_vertex_normal()
    {
        vec3 normal;
        normal.x = float((_vvdat_i.y >> 20U) & 0x3FFU) / 511.0 * 2.0 - 1.0;
        normal.y = float((_vvdat_i.y >> 10U) & 0x3FFU) / 511.0 * 2.0 - 1.0;
        normal.z = float((_vvdat_i.y >>  0U) & 0x3FFU) / 511.0 * 2.0 - 1.0;
        return normal;
    }

    // vvdat_i[2]: 2x16-bit texture coords
    vec2 voxel_vertex_texcoord()
    {
        vec2 texcoord;
        texcoord.x = float((_vvdat_i.z >> 16U) & 0xFFFFU) / 65535.0 * 32.0;
        texcoord.y = float((_vvdat_i.z >>  0U) & 0xFFFFU) / 65535.0 * 32.0;
        return texcoord;
    }

    // vvdat_i[3]: 1x16-bit toffset, 1x16-bit tframes
    #define voxel_vertex_toffset() ((_vvdat_i.w >> 16U) & 0xFFFFU)
    #define voxel_vertex_tframes() ((_vvdat_i.w >>  0U) & 0xFFFFU)
)glsl";

VoxelVertex::VoxelVertex(const vec3f_t &position, const vec3f_t &normal, const vec2f_t &uv, uint16_t toffset, uint16_t tframes)
{
    // vvdat_i[0]: 3x10-bit vertex position
    vvdat_i[0] |= (static_cast<uint32_t>(position.x / 16.0f * 511.0f) & 0x3FF) << 20;
    vvdat_i[0] |= (static_cast<uint32_t>(position.y / 16.0f * 511.0f) & 0x3FF) << 10;
    vvdat_i[0] |= (static_cast<uint32_t>(position.z / 16.0f * 511.0f) & 0x3FF);

    // vvdat_i[1]: 3x10-bit vertex normal
    vvdat_i[1] |= (static_cast<uint32_t>((0.5f + 0.5f * normal.x) * 511.0f) & 0x3FF) << 20;
    vvdat_i[1] |= (static_cast<uint32_t>((0.5f + 0.5f * normal.y) * 511.0f) & 0x3FF) << 10;
    vvdat_i[1] |= (static_cast<uint32_t>((0.5f + 0.5f * normal.z) * 511.0f) & 0x3FF);

    // vvdat_i[2]: 2x16-bit texture coords
    vvdat_i[2] |= (static_cast<uint32_t>(uv.x / 32.0f * 65535.0f) & 0xFFFF) << 16;
    vvdat_i[2] |= (static_cast<uint32_t>(uv.y / 32.0f * 65535.0f) & 0xFFFF);

    // vvdat_i[3]: 1x16-bit toffset, 1x16-bit tframes
    vvdat_i[3] |= 0xFFFF0000 & (toffset << 16);
    vvdat_i[3] |= 0x0000FFFF & (tframes);
}

void VoxelVertex::setup(glxx::VertexArray &vao)
{
    vao.enable_attribute(0, true);
    vao.set_attribute_format(0, GL_UNSIGNED_INT, 4, offsetof(VoxelVertex, vvdat_i), false);
    vao.set_attribute_binding(0, VOXEL_VERTEX_VBO_BINDING);
}

void VoxelVertex::init()
{
    shaders::add("voxel_vertex", GL_VERTEX_SHADER, voxel_vertex_vert);
}
