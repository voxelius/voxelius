// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

layout(location = 0) in vec2 vert_Position;

out vec2 vs_TexCoord;

uniform vec2 u_ScreenScale;
uniform float u_CurTime;

void main(void)
{
    vs_TexCoord = (0.5 + 0.5 * vert_Position + u_CurTime / 360.0) * u_ScreenScale;
    gl_Position.xy = vert_Position;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
