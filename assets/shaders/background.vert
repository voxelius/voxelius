// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common
#pragma import screenspace

layout(location = 0) out vec2 texcoord;

void main(void)
{
    texcoord = screenspace_texcoord;
    gl_Position.xy = screenspace_position;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
