// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

layout(location = 0) in vec3 vert_Position;

uniform mat4x4 u_ViewProjMatrix;
uniform mat4x4 u_ModelMatrix;

void main(void)
{
    gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vert_Position, 1.0);
}
