// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
#version 450 core
#pragma import common

layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 target;

layout(std140, binding = 0) uniform Background_UBO {
    vec4 time;
};

void main(void)
{
    target.x = 0.125 + 0.125 * cos(0.25 * time.x + texcoord.x * texcoord.y);
    target.y = 0.125 + 0.125 * sin(0.25 * time.x + texcoord.y * texcoord.y);
    target.z = 0.25;
    target.w = 1.00;
}
