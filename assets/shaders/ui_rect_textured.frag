// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common

layout(location = 0) in vec2 texcoord;

layout(location = 0) out vec4 target;

layout(binding = 0) uniform sampler2D image;

layout(std140, binding = 0) uniform RectDraw_UBO {
    vec4 screen;
    vec4 color;
    vec4 rect;
};

void main(void)
{
    target = color * texture(image, texcoord);
}

