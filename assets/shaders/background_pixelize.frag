// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

out vec4 frag_Target;

in vec2 vs_TexCoord;

uniform float u_Scale;
uniform vec2 u_Resolution;
uniform sampler2D u_Texture;

void main(void)
{
    vec2 pixel = 1.0f / u_Resolution;
    vec2 pixcoord = vs_TexCoord * u_Resolution;

    vec2 uv = vec2(0.0, 0.0);
    uv.x = u_Scale * pixel.x * floor(pixcoord.x / u_Scale);
    uv.y = u_Scale * pixel.y * floor(pixcoord.y / u_Scale);

    float mod_x = mod(pixcoord.x, u_Scale);
    float mod_y = mod(pixcoord.y, u_Scale);

    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    for(float i = 0.0; i < u_Scale; i += 1.0)
    for(float j = 0.0; j < u_Scale; j += 1.0)
        color += texture(u_Texture, uv + pixel * vec2(i, j));
    color /= u_Scale * u_Scale;

    float gap_width = u_Scale * 0.5;
    float gap_height = u_Scale * 0.5;

    if((mod_x <= gap_width) || (mod_y <= gap_height))
        frag_Target = vec4(0.0, 0.0, 0.0, 1.0);
    else frag_Target = color;
}
