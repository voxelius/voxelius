// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

layout(location = 0) in vec3 vert_Position;

uniform mat4x4 u_ViewProjMatrix;
uniform vec3 u_WorldPosition;
uniform vec3 u_Scale;

void main(void)
{
    vec3 positions[6];
    positions[0] = vec3(vert_Position.x, 0.0, vert_Position.z);
    positions[1] = vec3(vert_Position.x, 1.0 - vert_Position.z, 1.0);
    positions[2] = vec3(vert_Position.x, vert_Position.z, 0.0);
    positions[3] = vec3(1.0, 1.0 - vert_Position.x, vert_Position.z);
    positions[4] = vec3(0.0, vert_Position.x, vert_Position.z);
    positions[5] = vec3(vert_Position.x, 1.0, 1.0 - vert_Position.z);

    gl_Position.w = 1.0;
    gl_Position.xyz = positions[gl_InstanceID] * u_Scale + u_WorldPosition;
    gl_Position = u_ViewProjMatrix * gl_Position;
}
