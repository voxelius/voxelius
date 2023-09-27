// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#version 450 core

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 target;

layout(binding = 0) uniform sampler2D solid_albedo;
layout(binding = 1) uniform sampler2D solid_normal;

void main(void)
{
    // The oldest trick in the book.
    const vec3 IM_LIGHT_DIR = vec3(-0.500,  1.000, -0.250);
    const vec3 IM_AMBIENT_C = vec3( 0.200,  0.200,  0.200);

    // Get data from the gbuffer.
    const vec4 albedo = texture(solid_albedo, uv);
    const vec3 normal = texture(solid_normal, uv).xyz;

    // Calculate constant diffuse
    const float diffuse_c = max(dot(normalize(normal), IM_LIGHT_DIR), 0.0);

    // Combine lighting
    target = vec4(0.0, 0.0, 0.0, 1.0);
    target.rgb += albedo.rgb;
    target.rgb *= diffuse_c;
    target.rgb += IM_AMBIENT_C * albedo.rgb;
}
