// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#version 450 core
#pragma import common
#pragma import screenspace

layout(location = 0) out vec2 texcoord;

void main(void)
{
    texcoord = screenspace_texcoord;
    gl_Position.xy = screenspace_position;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
