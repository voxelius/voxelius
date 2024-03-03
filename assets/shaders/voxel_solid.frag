// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
#version 450 core
#pragma import common

layout(location = 0) in vec3 texcoord;
layout(location = 1) in float shade;

layout(location = 0) out vec4 target;

layout(binding = 0) uniform sampler2DArray textures;

void main(void)
{
    target = shade * texture(textures, texcoord);
}
