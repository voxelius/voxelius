// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

out vec4 frag_Target;

void main(void)
{
    frag_Target = vec4(1.0, 1.0, 1.0, 1.0);
    frag_Target *= round(mod(gl_FragCoord.x, 2.0));
    frag_Target *= round(mod(gl_FragCoord.y, 2.0));
}
