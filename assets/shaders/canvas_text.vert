// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

layout(location = 0) in vec2 vert_Position;
layout(location = 1) in uvec2 vert_Quad;

out vec2 vs_TexCoord;

uniform vec4 u_ScreenSize;
uniform vec2 u_Offset;
uniform vec2 u_AtlasSize;
uniform vec3 u_GlyphSize;

void main(void)
{
    vec2 offset;
    offset.x = u_GlyphSize.z * u_GlyphSize.x * float(0x0000FFFFU & (vert_Quad.x >> 16U));
    offset.y = u_GlyphSize.z * u_GlyphSize.y * float(0x0000FFFFU & (vert_Quad.x >> 0U));

    vs_TexCoord.x = (vert_Position.x + float(vert_Quad.y % uint(u_AtlasSize.x))) / u_AtlasSize.x;
    vs_TexCoord.y = 1.0 - (vert_Position.y + float(vert_Quad.y / uint(u_AtlasSize.x))) / u_AtlasSize.y;

    // The input coordinates are in screen-space coordinate system;
    // To correctly convert the coordinates to NDC, we need to flip Y axis
    gl_Position.x = 2.0 * (u_ScreenSize.z * (offset.x + u_GlyphSize.z * u_GlyphSize.x * vert_Position.x)) - 1.0;
    gl_Position.y = 1.0 - 2.0 * (u_ScreenSize.w * (offset.y + u_GlyphSize.z * u_GlyphSize.y * vert_Position.y));
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
