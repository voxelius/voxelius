// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core

in vertex_interface {
    vec3 texcoord;
    vec3 normal;
} vert;

layout(location = 0) out vec4 solid_albedo;
layout(location = 1) out vec4 solid_normal;

layout(binding = 0) uniform sampler2DArray textures;

void main(void)
{
    solid_albedo = texture(textures, vert.texcoord);
    solid_normal.xyz = normalize(vert.normal);
    solid_normal.w = 0.0;
}
