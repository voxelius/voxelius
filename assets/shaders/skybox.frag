// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core

in vec3 vs_Direction;

out vec4 frag_Target;

uniform vec4 u_Scatter;
#define ANISOTROPIC_INTENSITY u_Scatter.x
#define ATMOSPHERIC_DENSITY u_Scatter.y
#define MULTISCATTER_PHASE u_Scatter.z
#define ZENITH_OFFSET u_Scatter.w

uniform uvec3 u_Stars;
#define STARS_GAMMA u_Stars.x
#define STARS_ZOOM u_Stars.y
#define STARS_SEED u_Stars.z

uniform vec3 u_SkyColor;
uniform vec3 u_SunDirection;

#define as_absorption(pt, y) (2.0 * exp2((pt) * (-(y))))
#define as_zenith_density(x) (ATMOSPHERIC_DENSITY / pow(max((x) - ZENITH_OFFSET, 0.0035), 0.75))

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
    frag_Target.x = 0.0;
    frag_Target.y = 0.0;
    frag_Target.z = 0.0;
    frag_Target.w = 1.0;
   
    vec3 look = normalize(vs_Direction);

    float sun_point_dist_mult = clamp(u_SunDirection.y + MULTISCATTER_PHASE - ZENITH_OFFSET, 0.0, 1.0);
    float zenith = as_zenith_density(look.y);

    vec3 sky_color = u_SkyColor + u_SkyColor * ANISOTROPIC_INTENSITY;
    vec3 sun_absorption = as_absorption(sky_color, as_zenith_density(u_SunDirection.y + MULTISCATTER_PHASE));
    vec3 sky_absorption = as_absorption(sky_color, zenith);
    vec3 sky = sky_color * zenith;
    
    frag_Target.xyz += mix(sky * sky_absorption, sky / (sky + 0.5), sun_point_dist_mult);
    frag_Target.xyz += sky_absorption * 50.0 * smoothstep(0.050, 0.025, distance(look, u_SunDirection));
    frag_Target.xyz *= sun_absorption * 0.5 + 0.5 * length(sun_absorption);
    frag_Target.xyz *= 3.14159265359;

    // Tonemap the sky image
    float luminance = dot(frag_Target.xyz, vec3(0.2126, 0.7152, 0.0722));
    vec3 tcolor = (frag_Target.xyz / (frag_Target.xyz + 1.0));
    frag_Target.xyz = mix(frag_Target.xyz / (luminance + 1.0), tcolor, tcolor);
    frag_Target.xyz = pow(frag_Target.xyz, vec3(2.2));

    uint stars_x = uint(float(STARS_ZOOM) + float(STARS_ZOOM) * look.x);
    uint stars_y = uint(float(STARS_ZOOM) + float(STARS_ZOOM) * look.y);
    uint stars_z = uint(float(STARS_ZOOM) + float(STARS_ZOOM) * look.z);
    uint stars_hash = hash(stars_x + hash(stars_y) + hash(stars_z) + hash(STARS_SEED));
    float stars_value = uintBitsToFloat(0x3F800000U | (stars_hash >> 9)) - 1.0;
    float stars = clamp(pow(stars_value, float(STARS_GAMMA)), 0.0, 1.0);

    frag_Target.xyz = max(frag_Target.xyz, vec3(stars * (1.0 - (0.5 + 0.5 * u_SunDirection.y))));
}
