// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core
#pragma variant[0] ATMOSPHERIC_SCATTERING

layout(location = 0) in vec3 vert_Position;

out vec3 vs_Direction;

uniform mat4x4 u_ViewProjMatrix;
uniform vec3 u_WorldPosition;

void main(void)
{
    vs_Direction = normalize(vert_Position);
    gl_Position = u_ViewProjMatrix * vec4(u_WorldPosition + vert_Position, 1.0);
}
