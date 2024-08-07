// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

#define PAGE_SIZE 16U

in vec2 vs_TexCoord;        // Normalized UV coordinates
in vec3 vs_TexCoord_G;      // Atlas UV coordinates for a specific glyph
in vec4 vs_Background;      // Background color
in vec4 vs_Foreground;      // Foreground color
in vec3 vs_Attributes;      // Bold, strike-through and underline coefficients

uniform vec2 u_GlyphSize;   // X - unscaled height, Z - scale
uniform float u_Time;       // Current time in seconds (for randomizer)

uniform sampler2DArray u_Font;

out vec4 frag_Target;

void main(void)
{
    float glyph = 0.0;
    glyph += texture(u_Font, vs_TexCoord_G).x;
    glyph += texture(u_Font, vs_TexCoord_G - vec3(vs_Attributes.x / (u_GlyphSize.x * float(PAGE_SIZE)), 0.0, 0.0)).x;
    glyph += vs_Attributes.y * (step(vs_TexCoord.y, 0.6) - step(vs_TexCoord.y, 0.6 - 1.0 / u_GlyphSize.x));
    glyph += vs_Attributes.z * step(1.0 - vs_TexCoord.y, 1.0 / u_GlyphSize.x);
    
    //glyph += texture(u_Font, vs_GlyphCoord - vec2(vs_Attributes.x / u_GlyphSize.x / u_AtlasSize.x, 0.0)).x;
    frag_Target = mix(vs_Background, vs_Foreground, clamp(glyph, 0.0, 1.0));
}
