// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

#define PAGE_SIZE 16U

#define TEXT_RANDOM         0x0001U
#define TEXT_BOLD           0x0002U
#define TEXT_STRIKETHROUGH  0x0004U
#define TEXT_UNDERLINE      0x0008U
#define TEXT_ITALIC         0x0010U

layout(location = 0) in vec2 vert_Position;
layout(location = 1) in uvec4 vert_QuadData_1;
layout(location = 2) in uvec4 vert_QuadData_2;

out vec2 vs_TexCoord;       // Normalized UV coordinates
out vec3 vs_TexCoord_G;     // Atlas UV coordinates for a specific glyph
out vec4 vs_Background;     // Background color
out vec4 vs_Foreground;     // Foreground color
out vec3 vs_Attributes;     // Bold, strike-through and underline coefficients

uniform vec4 u_ScreenSize;  // XY - size, ZW - 1.0/size
uniform vec2 u_Offset;      // Pixel offset in screen coordinates
uniform vec2 u_GlyphSize;   // X - unscaled height, Z - scale
uniform float u_Time;       // Current time in seconds (for randomizer)
uniform float u_Tint;       // Tint (shadow); Applies shadow fixes when less than 1.0

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
    // Figure out glyph metrics
    // data_2[0] ----------------WWWWWWWWWWWWWWWW
    float glyph_width = float(0x0000FFFFU & (vert_QuadData_2.x >> 0U));

    // Figure out actual pixel offset
    // data_1[0] XXXXXXXXXXXXXXXXYYYYYYYYYYYYYYYY
    // [2024-08-05] now it's not relative to the glyph size
    vec2 offset;
    offset.x = u_Offset.x + u_GlyphSize.y * float(int(0x0000FFFFU & (vert_QuadData_1.x >> 16U)) - 0x8000);
    offset.y = u_Offset.y + u_GlyphSize.y * float(int(0x0000FFFFU & (vert_QuadData_1.x >> 0U)) - 0x8000);

    // Figure out scaled glyph size
    vec2 scaled_glyph;
    scaled_glyph.x = u_GlyphSize.y * glyph_width;
    scaled_glyph.y = u_GlyphSize.y * u_GlyphSize.x;

    // Figure out unicode value and attributes
    // data_1[1] TTTTTTTTUUUUUUUUUUUUUUUUUUUUUUUU
    uint attribs = 0x000000FFU & (vert_QuadData_1.y >> 24U);
    uint unicode = 0x00FFFFFFU & (vert_QuadData_1.y >> 0U);

    // The in-page coordinates can be randomized with TEXT_RANDOM
    uint unicode_inpage = unicode & 0x000000FFU;
    uint unicode_pagenum = unicode >> 8U;

    // Randomize the glyph that is drawn
    if((attribs & TEXT_RANDOM) != 0x00U) {
        unicode_inpage = hash(uint(gl_InstanceID) * uint(u_Time * 1000.0));
        unicode_inpage &= 0x000000FFU;
    }

    // The atlas is sampled twice in the fragment shader;
    // if the TEXT_BOLD attribute is set, second sample is offset
    if((attribs & TEXT_BOLD) != 0x00U)
        vs_Attributes.x = 1.0;
    else vs_Attributes.x = 0.0;

    // Add a horizontal line in the middle of a glyph
    if((attribs & TEXT_STRIKETHROUGH) != 0x00U)
        vs_Attributes.y = 1.0;
    else vs_Attributes.y = 0.0;

    // Add a horizontal line at the bottom of a glyph
    if((attribs & TEXT_UNDERLINE) != 0x00U)
        vs_Attributes.z = 1.0;
    else vs_Attributes.z = 0.0;

    // Distort the glyph diagonally
    if((attribs & TEXT_ITALIC) != 0x00U) {
        if(u_Tint < 1.0)
            offset.x -= 0.25 * u_GlyphSize.y;
        offset.x -= 0.5 * scaled_glyph.x * vert_Position.y;
    }

    vs_TexCoord = vert_Position;

    vs_TexCoord_G.x = float(unicode_inpage % PAGE_SIZE);
    vs_TexCoord_G.x += vert_Position.x * (glyph_width / u_GlyphSize.x);
    vs_TexCoord_G.x /= 16.0;
    
    /// vs_TexCoord_G.x = float(unicode_inpage % 16U) / 16.0;
    /// vs_TexCoord_G.x += vert_Position.x * 0.25 / 16.0;
    /// vs_TexCoord_G.x += 0.25 / 16.0;
    // vs_TexCoord_G.x *= 0.25; //glyph_width / u_GlyphSize.x;
    // vs_TexCoord_G.x += float(unicode_inpage % 16U);
    // vs_TexCoord_G.x /= 16.0;

    vs_TexCoord_G.y = 1.0 - (vert_Position.y + float(unicode_inpage / PAGE_SIZE)) / float(PAGE_SIZE);
    vs_TexCoord_G.z = floor(float(unicode_pagenum) + 0.5);

    // data_1[2] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA
    vs_Background.x = float(0x000000FFU & (vert_QuadData_1.z >> 24U)) / 255.0 * u_Tint;
    vs_Background.y = float(0x000000FFU & (vert_QuadData_1.z >> 16U)) / 255.0 * u_Tint;
    vs_Background.z = float(0x000000FFU & (vert_QuadData_1.z >> 8U))  / 255.0 * u_Tint;
    vs_Background.w = float(0x000000FFU & (vert_QuadData_1.z >> 0U))  / 255.0;

    // data_1[3] RRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA
    vs_Foreground.x = float(0x000000FFU & (vert_QuadData_1.w >> 24U)) / 255.0 * u_Tint;
    vs_Foreground.y = float(0x000000FFU & (vert_QuadData_1.w >> 16U)) / 255.0 * u_Tint;
    vs_Foreground.z = float(0x000000FFU & (vert_QuadData_1.w >> 8U))  / 255.0 * u_Tint;
    vs_Foreground.w = float(0x000000FFU & (vert_QuadData_1.w >> 0U))  / 255.0;

    // The input coordinates are in screen-space coordinate system;
    // To correctly convert the coordinates to NDC, we need to flip Y axis
    gl_Position.x = 2.0 * (u_ScreenSize.z * (offset.x + scaled_glyph.x * vert_Position.x)) - 1.0;
    gl_Position.y = 1.0 - 2.0 * (u_ScreenSize.w * (offset.y + scaled_glyph.y * vert_Position.y));
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
