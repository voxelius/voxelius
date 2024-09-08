// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core
#pragma variant[0] ATMOSPHERIC_SCATTERING

#define M_PI 3.14159265359
#define M_PI_2 1.57079632679

in vec3 vs_Direction;

out vec4 frag_Target;

uniform vec3 u_SunDirection;
uniform vec2 u_Horizon;

uint hash(uint x)
{
    x ^= x >> 0x010U;
    x *= 0x7FEB352DU;
    x ^= x >> 0x010U;
    x *= 0x846CA68BU;
    x ^= x >> 0x010U;
    return x;
}

void main(void)
{
    vec3 phase = vec3(0.0, 0.0, 0.0);
    phase.x = 1.0 - pow(2.0, (u_SunDirection.y + 0.125) * (-35.0));
    phase.y = 1.0 - pow(2.0, (u_SunDirection.y + 0.125) * (-10.0));
    phase.z = 1.0 - pow(2.0, (u_SunDirection.y + 0.125) * (-7.0));

    vec3 normalized_dir = normalize(vs_Direction);

    frag_Target.x = phase.x * pow(max(1.0 - smoothstep(-u_Horizon.x, u_Horizon.y, normalized_dir.y) * 0.25, 0.0), 6.0);
    frag_Target.y = phase.y * pow(max(1.0 - smoothstep(-u_Horizon.x, u_Horizon.y, normalized_dir.y) * 0.25, 0.0), 3.0);
    frag_Target.z = phase.z * pow(max(1.0 - smoothstep(-u_Horizon.x, u_Horizon.y, normalized_dir.y) * 0.25, 0.0), 1.5);
    frag_Target.w = 1.0;

    float sun_disk = smoothstep(0.0625, 0.0500, distance(normalized_dir, u_SunDirection));
    float sun_dot = clamp(dot(normalized_dir, u_SunDirection), 0.0, 1.0);
    frag_Target.xyz += phase * pow(sun_dot, 64.0) * 0.25;
    frag_Target.xyz += phase * sun_disk;

    uint stx = uint(256.0 + 256.0 * normalized_dir.x);
    uint sty = uint(256.0 + 256.0 * normalized_dir.y);
    uint stz = uint(256.0 + 256.0 * normalized_dir.z);
    uint stv = hash(stx + hash(sty) + hash(stz));
    float stars = clamp(pow((uintBitsToFloat(0x3F800000U | (stv >> 9)) - 1.0), 1000.0 * (1.2 + 1.0)), 0.0, 1.0);

    frag_Target.xyz = max(frag_Target.xyz, stars * vec3(1.0 - (0.5 + 0.5 * u_SunDirection.y)));
}
