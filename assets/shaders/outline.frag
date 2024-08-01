// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

out vec4 frag_Target;

uniform vec4 u_Color;

void main(void)
{
    frag_Target = u_Color;
}
