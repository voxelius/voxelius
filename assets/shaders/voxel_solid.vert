// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common
#pragma import voxel_anims
#pragma import voxel_vertex

layout(location = 0) out vec3 texcoord;
layout(location = 1) out float shade;

layout(std140, binding = 0) uniform VoxelRender_UBO {
    mat4x4 viewmat;
    uvec4 timings;
    vec4 chunk;
};

void main(void)
{
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    gl_Position.xyz += chunk.xyz;
    gl_Position.xyz += voxel_vertex_position();
    gl_Position = viewmat * gl_Position;

    const uint toffset = voxel_vertex_toffset();
    const uint tframes = voxel_vertex_tframes();
    const uint atex = voxel_anims[toffset + (timings.x % tframes)];

    texcoord.xy = voxel_vertex_texcoord();
    texcoord.z = floor(float(atex) + 0.5);
    shade = voxel_vertex_shade();
}
