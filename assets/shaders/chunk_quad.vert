// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core
#pragma variant[0] WORLD_CURVATURE
#pragma variant[1] WORLD_FOG

layout(location = 0) in vec3 vert_Position;
layout(location = 1) in uvec2 vert_Quad;

out vec3 vs_TexCoord;
out float vs_Shade;

#if WORLD_FOG
out float vs_FogFactor;
#endif

uniform mat4x4 u_ViewProjMatrix;
uniform vec3 u_WorldPosition;
uniform uvec3 u_Timings;
uniform float u_ViewDistance;

void main(void)
{
    vec3 quad_offset;
    quad_offset.x = float(0x00FFU & (vert_Quad.x >> 24U)) / 16.0;
    quad_offset.y = float(0x00FFU & (vert_Quad.x >> 16U)) / 16.0;
    quad_offset.z = float(0x00FFU & (vert_Quad.x >> 8U))  / 16.0;

    vec2 quad_scale;
    quad_scale.x = float((0x000FU & (vert_Quad.x >> 4U)) + 1U) / 16.0;
    quad_scale.y = float((0x000FU & (vert_Quad.x >> 0U)) + 1U) / 16.0;

    uint quad_facing = (0x000FU & (vert_Quad.y >> 28U));
    uint quad_toffset = (0x07FFU & (vert_Quad.y >> 17));
    uint quad_tframes = max(1U, (0x001FU & (vert_Quad.y >> 12)));

    gl_Position.xyz = vert_Position;
    gl_Position.x *= quad_scale.x;
    gl_Position.z *= quad_scale.y;

    vec3 positions[6]; // FIXME: 16
    positions[0x00U] = vec3(gl_Position.x, 1.0 - gl_Position.z, 1.0);
    positions[0x01U] = vec3(gl_Position.x, gl_Position.z, 0.0);
    positions[0x02U] = vec3(1.0, 1.0 - gl_Position.x, gl_Position.z);
    positions[0x03U] = vec3(0.0, gl_Position.x, gl_Position.z);
    positions[0x04U] = vec3(gl_Position.x, 1.0, gl_Position.z);
    positions[0x05U] = vec3(gl_Position.x, 0.0, 1.0 - gl_Position.z);

    vec2 texcoords[6]; // FIXME: 16
    texcoords[0x00U] = vec2(vert_Position.x, 1.0 - vert_Position.z);
    texcoords[0x01U] = vec2(1.0 - vert_Position.x, vert_Position.z);
    texcoords[0x02U] = vec2(-vert_Position.z, 1.0 - vert_Position.x);
    texcoords[0x03U] = vec2(1.0 + vert_Position.z, vert_Position.x);
    texcoords[0x04U] = vec2(1.0 - vert_Position.x, vert_Position.z);
    texcoords[0x05U] = vec2(1.0 - vert_Position.x, vert_Position.z);

    float shades[6]; // FIXME: 16
    shades[0x00U] = 0.8;
    shades[0x01U] = 0.8;
    shades[0x02U] = 0.6;
    shades[0x03U] = 0.6;
    shades[0x04U] = 1.0;
    shades[0x05U] = 0.4;

    vs_TexCoord.xy = texcoords[quad_facing];
    vs_TexCoord.z = floor(float(quad_toffset + u_Timings.z % quad_tframes) + 0.5);
    vs_Shade = shades[quad_facing];

    gl_Position.w = 1.0;
    gl_Position.xyz = positions[quad_facing] + quad_offset + u_WorldPosition;
    gl_Position = u_ViewProjMatrix * gl_Position;

#if WORLD_FOG == 1
    // Use a simple linear fog factor
    vs_FogFactor = 1.0 - clamp((u_ViewDistance - length(gl_Position.xyz)) / (u_ViewDistance - 16.0), 0.0, 1.0);;
#elif WORLD_FOG == 2
    // Use a fancy exponential fog factor
    // that is totally not yoinked from CaveCube
    // UNDONE: for now it's just a boring exponential fog
    float fogd = 2.0 / u_ViewDistance * length(gl_Position.xyz);
    vs_FogFactor = 1.0 - clamp(exp2(fogd * fogd * -1.442695), 0.0, 1.0);
#endif

#if WORLD_CURVATURE
    gl_Position.y += gl_Position.z * gl_Position.z / u_ViewDistance;
    gl_Position.y += gl_Position.x * gl_Position.x / u_ViewDistance;
#endif
}
