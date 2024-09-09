// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

#define M_PI 3.14159265359
#define M_PI_2 1.57079632679

in vec3 vs_Direction;

out vec4 frag_Target;

uniform vec3 u_Absorption;
#define ABSORPTION_R u_Absorption.x
#define ABSORPTION_G u_Absorption.y
#define ABSORPTION_B u_Absorption.z

uniform vec3 u_Horizon;
#define HORIZON_SCALE u_Horizon.x
#define HORIZON_START u_Horizon.y
#define HORIZON_END u_Horizon.z

uniform vec3 u_Stars;
#define STARS_GAMMA u_Stars.x
#define STARS_SCALE u_Stars.y
#define STARS_SEED u_Stars.z

uniform vec3 u_SunDirection;
uniform float u_TwilightOffset;

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
    phase.x = clamp(1.0 - pow(2.0, (u_SunDirection.y + u_TwilightOffset) * (-ABSORPTION_R)), 0.0, 1.0);
    phase.y = clamp(1.0 - pow(2.0, (u_SunDirection.y + u_TwilightOffset) * (-ABSORPTION_G)), 0.0, 1.0);
    phase.z = clamp(1.0 - pow(2.0, (u_SunDirection.y + u_TwilightOffset) * (-ABSORPTION_B)), 0.0, 1.0);

    vec3 normalized_dir = normalize(vs_Direction);
    frag_Target.x = phase.x * pow(max(1.0 - smoothstep(HORIZON_START, HORIZON_END, normalized_dir.y) * HORIZON_SCALE, 0.0), 6.0);
    frag_Target.y = phase.y * pow(max(1.0 - smoothstep(HORIZON_START, HORIZON_END, normalized_dir.y) * HORIZON_SCALE, 0.0), 3.0);
    frag_Target.z = phase.z * pow(max(1.0 - smoothstep(HORIZON_START, HORIZON_END, normalized_dir.y) * HORIZON_SCALE, 0.0), 1.5);
    frag_Target.w = 1.0;

    // FIXME: render the sun as a 2D billboard sprite
    float sun_dot = clamp(dot(normalized_dir, u_SunDirection), 0.0, 1.0);
    frag_Target.xyz += 0.25 * phase * pow(sun_dot, 64.0);

    uint stars_x = uint(STARS_SCALE + STARS_SCALE * normalized_dir.x);
    uint stars_y = uint(STARS_SCALE + STARS_SCALE * normalized_dir.y);
    uint stars_z = uint(STARS_SCALE + STARS_SCALE * normalized_dir.z);
    uint stars_hash = hash(stars_x + hash(stars_y) + hash(stars_z) + uint(STARS_SEED));
    float stars_value = uintBitsToFloat(0x3F800000U | (stars_hash >> 9)) - 1.0;
    float stars = clamp(pow(stars_value, 1000.0 * STARS_GAMMA), 0.0, 1.0);

    frag_Target.xyz = max(frag_Target.xyz, vec3(stars * (1.0 - (0.5 + 0.5 * u_SunDirection.y))));
}
