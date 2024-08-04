// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

#define TEXT_RANDOM         0x0001U
#define TEXT_BOLD           0x0002U
#define TEXT_STRIKETHROUGH  0x0004U
#define TEXT_UNDERLINE      0x0008U
#define TEXT_ITALIC         0x0010U

layout(location = 0) in vec2 vert_Position;
layout(location = 1) in uvec4 vert_Quad;

out vec2 vs_TexCoord;
out vec2 vs_GlyphCoord;
out vec4 vs_Background;
out vec4 vs_Foreground;
out float vs_Oversample;
out float vs_Strikethrough;
out float vs_Underline;

uniform vec4 u_ScreenSize;
uniform vec2 u_Offset;
uniform vec2 u_AtlasSize;
uniform vec3 u_GlyphSize;
uniform float u_Time;
uniform float u_Tint;

uint hash(uint x)
{
    x ^= x >> 0x010U;
    x *= 0x7FEB352DU;
    x ^= x >> 0x010U;
    x *= 0x846CA68BU;
    x ^= x >> 0x010U;
    return x;
}

void main(void)
{
    vec2 offset;
    offset.x = u_Offset.x + u_GlyphSize.z * u_GlyphSize.x * float(0x0000FFFFU & (vert_Quad.x >> 16U));
    offset.y = u_Offset.y + u_GlyphSize.z * u_GlyphSize.y * float(0x0000FFFFU & (vert_Quad.x >> 0U));

    uint attributes = 0x000000FFU & (vert_Quad.y >> 24U);
    uint unicode = 0x00FFFFFFU & (vert_Quad.y >> 0U);

    if((attributes & TEXT_RANDOM) != 0U) {
        unicode = hash(uint(gl_InstanceID) * uint(u_Time * 1000.0)) & 0xFFU;
        unicode %= uint(u_AtlasSize.x * u_AtlasSize.y);
    }

    if((attributes & TEXT_BOLD) != 0U)
        vs_Oversample = 1.0;
    else vs_Oversample = 0.0;

    if((attributes & TEXT_STRIKETHROUGH) != 0U)
        vs_Strikethrough = 1.0;
    else vs_Strikethrough = 0.0;

    if((attributes & TEXT_UNDERLINE) != 0U)
        vs_Underline = 1.0;
    else vs_Underline = 0.0;

    if((attributes & TEXT_ITALIC) != 0U) {
        if(u_Tint < 1.0)
            offset.x -= 0.25 * u_GlyphSize.z;
        offset.x -= 0.5 * u_GlyphSize.z * u_GlyphSize.x * vert_Position.y;
    }

    vs_TexCoord = vert_Position;
    vs_GlyphCoord.x = (vert_Position.x + float(unicode % uint(u_AtlasSize.x))) / u_AtlasSize.x;
    vs_GlyphCoord.y = 1.0 - (vert_Position.y + float(unicode / uint(u_AtlasSize.x))) / u_AtlasSize.y;

    vs_Background.x = float(0x000000FFU & (vert_Quad.z >> 24U)) / 255.0 * u_Tint;
    vs_Background.y = float(0x000000FFU & (vert_Quad.z >> 16U)) / 255.0 * u_Tint;
    vs_Background.z = float(0x000000FFU & (vert_Quad.z >> 8U))  / 255.0 * u_Tint;
    vs_Background.w = float(0x000000FFU & (vert_Quad.z >> 0U))  / 255.0;

    vs_Foreground.x = float(0x000000FFU & (vert_Quad.w >> 24U)) / 255.0 * u_Tint;
    vs_Foreground.y = float(0x000000FFU & (vert_Quad.w >> 16U)) / 255.0 * u_Tint;
    vs_Foreground.z = float(0x000000FFU & (vert_Quad.w >> 8U))  / 255.0 * u_Tint;
    vs_Foreground.w = float(0x000000FFU & (vert_Quad.w >> 0U))  / 255.0;

    // The input coordinates are in screen-space coordinate system;
    // To correctly convert the coordinates to NDC, we need to flip Y axis
    gl_Position.x = 2.0 * (u_ScreenSize.z * (offset.x + u_GlyphSize.z * u_GlyphSize.x * vert_Position.x)) - 1.0;
    gl_Position.y = 1.0 - 2.0 * (u_ScreenSize.w * (offset.y + u_GlyphSize.z * u_GlyphSize.y * vert_Position.y));
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
