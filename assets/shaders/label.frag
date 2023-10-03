// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common

layout(location = 0) in vec2 texcoord;
layout(location = 1) in vec2 pixcoord;

layout(location = 0) out vec4 target;

layout(binding = 0) uniform sampler2D font;
layout(binding = 1) uniform usampler2D label;

layout(std140, binding = 0) uniform LabelRender_UBO {
    vec4 screen;
    vec4 glyph;
    vec4 color;
    vec4 rect;
};

void main(void)
{
    const vec2 guv = mod(pixcoord, glyph.xy) / glyph.xy / glyph.zw;
    const uint unicode = texture(label, texcoord).x;
    const float unicode_x = float(0U + unicode % uint(glyph.z)) / glyph.z;
    const float unicode_y = float(1U + unicode / uint(glyph.z)) / glyph.w;
    target = color * texture(font, guv + vec2(unicode_x, 1.0 - unicode_y)).x;
}
