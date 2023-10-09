// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/render/shaders.hh>
#include <client/render/voxel_vertex.hh>
#include <shared/cxmath.hh>

constexpr static const char *voxel_vertex_vert = R"glsl(
    layout(location = 0) in uvec4 _vvdat_i;

    // vvdat_i[0]: 3x10-bit vertex position
    vec3 voxel_vertex_position()
    {
        vec3 position;
        position.x = float((_vvdat_i.x >> 22U) & 0x3FFU) / 768.0 * 16.0;
        position.y = float((_vvdat_i.x >> 12U) & 0x3FFU) / 768.0 * 16.0;
        position.z = float((_vvdat_i.x >>  2U) & 0x3FFU) / 768.0 * 16.0;
        return position;
    }

    // vvdat_i[0]: 1x2-bit vertex shade
    #define voxel_vertex_shade() (float((_vvdat_i.x & 0x03U) + 2U) / 5.0)

    // vvdat_i[1]: 3x10-bit vertex normal
    vec3 voxel_vertex_normal()
    {
        vec3 normal;
        normal.x = float((_vvdat_i.y >> 22U) & 0x3FFU) / 768.0 * 2.0 - 1.0;
        normal.y = float((_vvdat_i.y >> 12U) & 0x3FFU) / 768.0 * 2.0 - 1.0;
        normal.z = float((_vvdat_i.y >>  2U) & 0x3FFU) / 768.0 * 2.0 - 1.0;
        return normal;
    }

    // vvdat_i[2]: 1x16-bit toffset
    // vvdat_i[2]: 1x16-bit tframes
    #define voxel_vertex_toffset() ((_vvdat_i.z >> 16U) & 0xFFFFU)
    #define voxel_vertex_tframes() ((_vvdat_i.z >>  0U) & 0xFFFFU)

    // vvdat_i[3]: 2x16-bit texture coordinates
    vec2 voxel_vertex_texcoord()
    {
        vec2 texcoord;
        texcoord.x = float((_vvdat_i.w >> 16U) & 0xFFFFU) / 65535.0 * 16.0;
        texcoord.y = float((_vvdat_i.w >>  0U) & 0xFFFFU) / 65535.0 * 16.0;
        return texcoord;
    }
)glsl";

VoxelVertex::VoxelVertex(const vector3_t &position, unsigned int shade, const vector3_t &normal, uint16_t toffset, uint16_t tframes, const vector2_t &uv)
{
    // vvdat_i[0]: 3x10-bit vertex position
    vvdat_i[0] |= (static_cast<uint32_t>(position.x / 16.0 * 768.0) & 0x3FF) << 22;
    vvdat_i[0] |= (static_cast<uint32_t>(position.y / 16.0 * 768.0) & 0x3FF) << 12;
    vvdat_i[0] |= (static_cast<uint32_t>(position.z / 16.0 * 768.0) & 0x3FF) <<  2;

    // vvdat_i[0]: 1x2-bit vertex shade
    vvdat_i[0] |= (static_cast<uint32_t>(shade) & 0x03);

    // vvdat_i[1]: 3x10-bit vertex normal
    vvdat_i[1] |= (static_cast<uint32_t>((0.5 + 0.5 * normal.x) * 768.0) & 0x3FF) << 22;
    vvdat_i[1] |= (static_cast<uint32_t>((0.5 + 0.5 * normal.y) * 768.0) & 0x3FF) << 12;
    vvdat_i[1] |= (static_cast<uint32_t>((0.5 + 0.5 * normal.z) * 768.0) & 0x3FF) <<  2;

    // vvdat_i[2]: 1x16-bit toffset
    // vvdat_i[2]: 1x16-bit tframes
    vvdat_i[2] |= 0xFFFF0000 & (toffset << 16);
    vvdat_i[2] |= 0x0000FFFF & (tframes);
 
    // vvdat_i[3]: 2x16-bit texture coordinates
    vvdat_i[3] |= (static_cast<uint32_t>(uv.x / 16.0 * 65535.0) & 0xFFFF) << 16;
    vvdat_i[3] |= (static_cast<uint32_t>(uv.y / 16.0 * 65535.0) & 0xFFFF);
}

void VoxelVertex::setup(gl::VertexArray &vao)
{
    vao.enable_attribute(0, true);
    vao.set_attribute_format(0, GL_UNSIGNED_INT, 4, offsetof(VoxelVertex, vvdat_i), false);
    vao.set_attribute_binding(0, VOXEL_VBO_BINDING);
}

void VoxelVertex::init()
{
    shaders::add("voxel_vertex", GL_VERTEX_SHADER, voxel_vertex_vert);
}
