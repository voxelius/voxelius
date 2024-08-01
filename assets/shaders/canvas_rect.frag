// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec4 vs_Color;

out vec4 frag_Target;

void main(void)
{
    frag_Target = vs_Color;
}
