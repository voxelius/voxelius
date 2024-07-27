// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec2 vs_TexCoord;

out vec4 frag_Target;

uniform float u_Time;

void main(void)
{
    frag_Target.x = 0.125 + 0.125 * cos(0.25 * u_Time + vs_TexCoord.x * vs_TexCoord.y);
    frag_Target.y = 0.125 + 0.125 * sin(0.25 * u_Time + vs_TexCoord.y * vs_TexCoord.y);
    frag_Target.z = 0.250;
    frag_Target.w = 1.000;
}
