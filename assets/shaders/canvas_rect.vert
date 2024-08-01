// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

layout(location = 0) in vec2 vert_Position;

out vec4 vs_Color;

uniform vec4 u_ScreenSize;
uniform vec2 u_Offset;
uniform vec2 u_Size;
uniform vec4 u_Color[4];

void main(void)
{
    vs_Color = u_Color[gl_VertexID % 4];

    // The input coordinates are in screen-space coordinate system;
    // To correctly convert the coordinates to NDC, we need to flip Y axis
    gl_Position.x = 2.0 * (u_ScreenSize.z * (u_Offset.x + u_Size.x * vert_Position.x)) - 1.0;
    gl_Position.y = 1.0 - 2.0 * (u_ScreenSize.w * (u_Offset.y + u_Size.y * vert_Position.y));
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
