// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec2 vs_TexCoord;

out vec4 frag_Target;

uniform sampler2D u_Texture;

void main(void)
{
    frag_Target = texture(u_Texture, vs_TexCoord);
}
