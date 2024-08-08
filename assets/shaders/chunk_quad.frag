// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec3 vs_TexCoord;
in float vs_Shade;
in float vs_FogCoeff;

out vec4 frag_Target;

uniform vec4 u_SkyColor;
uniform sampler2DArray u_Textures;

void main(void)
{
    frag_Target = vs_Shade * texture(u_Textures, vs_TexCoord);
    frag_Target = mix(frag_Target, u_SkyColor, vs_FogCoeff);
}
