// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

layout(location = 0) in vec2 vert_Position;

out vec2 vs_TexCoord;
out vec2 vs_PixCoord;
out vec4 vs_ColorMod;

uniform vec4 u_Color0;
uniform vec4 u_Color1;
uniform vec4 u_Color2;
uniform vec4 u_Color3;
uniform vec4 u_Screen;
uniform vec4 u_Rect;

void main(void)
{
    vec4 colormods[4];
    colormods[0] = u_Color0;
    colormods[1] = u_Color1;
    colormods[2] = u_Color2;
    colormods[3] = u_Color3;

    vs_TexCoord = 0.5 + 0.5 * vert_Position;
    vs_PixCoord = vs_TexCoord * u_Screen.xy;
    vs_ColorMod = colormods[gl_VertexID];

    gl_Position.xy = 0.5 + 0.5 * vec2(vert_Position.x, -vert_Position.y);
    gl_Position.xy = gl_Position.xy * u_Rect.zw * u_Screen.zw;
    gl_Position.xy = gl_Position.xy + u_Rect.xy * u_Screen.zw;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
