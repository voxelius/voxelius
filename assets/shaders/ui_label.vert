// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common
#pragma import screenspace

layout(location = 0) out vec2 texcoord;
layout(location = 1) out vec2 pixcoord;

layout(std140, binding = 0) uniform LabelDraw_UBO {
    vec4 screen;
    vec4 glyph;
    vec4 color;
    vec4 rect;
};

void main(void)
{
    const vec2 oa = vec2(-rect.z, -rect.w);
    const vec2 ob = vec2(+rect.z, -rect.w);
    const vec2 oc = vec2(+rect.z, +rect.w);
    const vec2 od = vec2(-rect.z, +rect.w);
    const vec2 ox[6] = { oa, ob, oc, oc, od, oa };

    const vec2 off = ox[gl_VertexID % 6];
    const vec2 ndc = 2.0 * screen.zw * (rect.xy + 0.5 * off);

    texcoord = screenspace_texcoord;
    pixcoord = texcoord * rect.zw;
    gl_Position = vec4(ndc.x, ndc.y, 0.0, 1.0);
}

