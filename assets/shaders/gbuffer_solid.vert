// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core

layout(location = 0) in vec3 vtx_position;
layout(location = 1) in vec3 vtx_norm;
layout(location = 2) in vec2 vtx_tex_uv;
layout(location = 3) in uint vtx_tex_id;

out vertex_interface {
    vec3 texcoord;
    vec3 normal;
} vert;

layout(std140, binding = 1) uniform VoxelRender_UBO {
    mat4x4 vpmat;
    vec4 chunk;
};

void main(void)
{
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    gl_Position.xyz += chunk.xyz;
    gl_Position.xyz += vtx_position;
    gl_Position = vpmat * gl_Position;

    vert.texcoord.x = vtx_tex_uv.x;
    vert.texcoord.y = vtx_tex_uv.y;
    vert.texcoord.z = floor(float(vtx_tex_id) + 0.5);
    vert.normal = vtx_norm;
}
