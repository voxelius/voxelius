// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

out vec4 frag_Target;

uniform vec3 u_Color;

void main(void)
{
    frag_Target.x = u_Color.x;
    frag_Target.y = u_Color.y;
    frag_Target.z = u_Color.z;
    frag_Target.w = 1.0;
}
