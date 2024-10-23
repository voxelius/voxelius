// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

layout(location = 0) in vec2 vert_Position;

out vec2 vs_TexCoord;

uniform vec2 u_Scale;

void main(void)
{
    vs_TexCoord = vert_Position * u_Scale;
    gl_Position.xy = 2.0 * vert_Position - 1.0;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
