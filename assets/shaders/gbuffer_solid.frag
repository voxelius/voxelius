// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core
#pragma import common

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 texcoord;

layout(location = 0) out vec4 target_albedo;
layout(location = 1) out vec4 target_normal;

layout(binding = 0) uniform sampler2DArray textures;

void main(void)
{
    target_albedo = texture(textures, texcoord);
    target_normal.xyz = normalize(normal);
    target_normal.w = 0.0;
}
