// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec2 vs_TexCoord;
in vec4 vs_Background;
in vec4 vs_Foreground;

out vec4 frag_Target;

uniform sampler2D u_Font;

void main(void)
{
    frag_Target = mix(vs_Background, vs_Foreground, texture(u_Font, vs_TexCoord).x);
}
