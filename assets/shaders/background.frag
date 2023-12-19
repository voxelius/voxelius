// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common

layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 target;

layout(std140, binding = 0) uniform Background_UBO {
    vec4 color;
    vec4 time;
};

void main(void)
{
    target.x = 0.5 + 0.5 * cos(0.125 * M_PI * time.x + texcoord.x);
    target.y = 0.5;
    target.z = 0.5 + 0.5 * sin(0.125 * M_PI * time.x + texcoord.y);
    target.w = 1.0;
}
