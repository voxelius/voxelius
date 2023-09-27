// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core

layout(location = 0) out vec2 uv;

const vec2 verts[8] = {
    vec2(-1.0, -1.0), vec2(+1.0, -1.0), vec2(+1.0, +1.0), vec2(-1.0, -1.0),
    vec2(+1.0, +1.0), vec2(-1.0, +1.0), vec2(-1.0, -1.0), vec2(+1.0, +1.0),
};

void main(void)
{
    uv = vec2(0.5, 0.5) + 0.5 * verts[gl_VertexID];
    gl_Position.xy = verts[gl_VertexID];
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
