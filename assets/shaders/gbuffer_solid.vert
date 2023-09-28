// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common
#pragma import voxel_anims
#pragma import voxel_vertex

inout vertex_interface {
    vec3 norm;
    vec3 uvw;
} vert;

layout(std140, binding = 1) uniform VoxelRender_UBO {
    mat4x4 vpmat;
    vec4 chunk;
};

void main(void)
{
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    gl_Position.xyz += chunk.xyz;
    gl_Position.xyz += voxel_vertex_position();
    gl_Position = vpmat * gl_Position;

    // FIXME
    const uint frame = 0U;

    const uint tex_offset = voxel_vertex_toffset();
    const uint tex_size = voxel_vertex_tsize();
    const uint tex = voxel_anims[tex_offset + (frame % tex_size)];

    vert.norm = voxel_vertex_norm();
    vert.uvw.xy = voxel_vertex_uv();
    vert.uvw.z = floor(float(tex) + 0.5);
}
