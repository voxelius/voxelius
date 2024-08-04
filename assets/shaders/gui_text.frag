// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec2 vs_TexCoord;
in vec2 vs_GlyphCoord;
in vec4 vs_Background;
in vec4 vs_Foreground;
in float vs_Oversample;
in float vs_Strikethrough;
in float vs_Underline;

out vec4 frag_Target;

uniform float u_Time;
uniform vec2 u_AtlasSize;
uniform vec3 u_GlyphSize;
uniform sampler2D u_Font;

void main(void)
{
    float glyph = 0.0;
    glyph += texture(u_Font, vs_GlyphCoord + vec2(0.0, 0.0)).x;
    glyph += texture(u_Font, vs_GlyphCoord - vec2(vs_Oversample / u_GlyphSize.x / u_AtlasSize.x, 0.0)).x;
    glyph += vs_Strikethrough * (step(vs_TexCoord.y, 0.5 - 0.0 / u_GlyphSize.y) - step(vs_TexCoord.y, 0.5 - 1.0 / u_GlyphSize.y));
    glyph += vs_Underline * step(1.0 - vs_TexCoord.y, 1.0 / u_GlyphSize.y);

    frag_Target = mix(vs_Background, vs_Foreground, clamp(glyph, 0.0, 1.0));
}
