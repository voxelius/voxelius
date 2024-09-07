// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core
#pragma variant[0] WORLD_CURVATURE
#pragma variant[1] WORLD_FOG

in vec3 vs_TexCoord;
in float vs_Shade;

#if WORLD_FOG
in float vs_FogFactor;
#endif

out vec4 frag_Target;

uniform vec4 u_FogColor;
uniform sampler2DArray u_Textures;

void main(void)
{
    frag_Target = vs_Shade * texture(u_Textures, vs_TexCoord);

#if WORLD_FOG
    frag_Target = mix(frag_Target, u_FogColor, vs_FogFactor);
#endif
}
