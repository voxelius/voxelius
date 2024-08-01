// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec2 vs_TexCoord;

out vec4 frag_Target;

uniform vec4 u_BackgroundColor;
uniform vec4 u_ForegroundColor;
uniform sampler2D u_Font;

void main(void)
{
    frag_Target = mix(u_BackgroundColor, u_ForegroundColor, texture(u_Font, vs_TexCoord).x);
}
