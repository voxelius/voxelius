// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

#define PAGE_SIZE 16U

in vec2 vs_TexCoord;        // Normalized UV coordinates
in vec3 vs_TexCoord_G;      // Atlas UV coordinates for a specific glyph
in vec4 vs_Background;      // Background color
in vec4 vs_Foreground;      // Foreground color
in vec3 vs_Attributes;      // Bold, strike-through and underline coefficients
in vec4 vs_Metrics;         // X - ascender, Y - descender, Z - advance, W - vertical offset

uniform vec2 u_GlyphSize;   // X - unscaled height, Z - scale
uniform float u_Time;       // Current time in seconds (for randomizer)

uniform sampler2DArray u_Font;

out vec4 frag_Target;

void main(void)
{
    float glyph = 0.0;
    glyph += texture(u_Font, vs_TexCoord_G).x;
    
    // Apply oversample (bold)
    glyph += vs_Attributes.x * texture(u_Font, vs_TexCoord_G - vec3((vs_Attributes.x / (u_GlyphSize.x * float(PAGE_SIZE))), 0.0, 0.0)).x;
    
    // Apply strikethrough (crossed)
    float glyph_bias = vs_Metrics.w / u_GlyphSize.x;
    float glyph_height = vs_Metrics.x + vs_Metrics.y;
    float actual_midpoint = 0.5 * glyph_height / u_GlyphSize.x - glyph_bias;
    glyph += vs_Attributes.y * (step(actual_midpoint, 1.0 - vs_TexCoord.y) - step(actual_midpoint + 1.0 / u_GlyphSize.x, 1.0 - vs_TexCoord.y));

    // TODO: underline

    frag_Target = mix(vs_Background, vs_Foreground, clamp(glyph, 0.0, 1.0));
}
