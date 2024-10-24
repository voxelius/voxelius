// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec2 vs_TexCoord;
in vec2 vs_TileCoord;

out vec4 frag_Target;

uniform sampler2D u_Texture;
uniform sampler2D u_RTexture;

void main(void)
{
    vec4 cube = texture(u_RTexture, vs_TexCoord);
    vec4 tile = texture(u_Texture, vs_TileCoord);
    frag_Target = clamp(tile + cube, 0.0, 1.0);
}
