// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#version 330 core
#pragma variant[0] ATMOSPHERIC_SCATTERING

#define M_PI 3.14159265359
#define M_PI_2 1.57079632679

in vec3 vs_Direction;

out vec4 frag_Target;

uniform vec3 u_BaseSkyColor;
uniform vec3 u_SunDirection;
uniform vec4 u_ScatterParams;
#define ANISOTROPIC_INTENSITY u_ScatterParams.x
#define ATMOSPHERIC_DENSITY u_ScatterParams.y
#define MULTISCATTER_PHASE u_ScatterParams.z
#define ZENITH_OFFSET u_ScatterParams.w

#define get_zenith_density(x) (ATMOSPHERIC_DENSITY / pow(max(((x) - ZENITH_OFFSET), 0.0035), 0.75))
#define get_sky_absorption(x, y) (2.0 * exp2((x) * (-(y))))
#define get_sun_point(p, lp) (50.0 * smoothstep(0.04, 0.03, distance((p), (lp))))
#define get_rayleigh(p, lp) (1.0 + M_PI_2 * pow(1.0 - clamp(distance((p), (lp)), 0.0, 1.0), 2.0))

vec3 scatter(in vec3 p, in vec3 lp)
{
    float zenith = get_zenith_density(p.y);
    float sun_point_dist_mult = clamp(length(max(lp.y + MULTISCATTER_PHASE - ZENITH_OFFSET, 0.0)), 0.0, 1.0);
    float rayleigh_mult = get_rayleigh(p, lp);

    // Base sky color that has at least one non-zero component
    vec3 base_sky = u_BaseSkyColor * (1.0 + ANISOTROPIC_INTENSITY);

    vec3 sky_absorption = get_sky_absorption(base_sky, zenith);
    vec3 sun_absorption = get_sky_absorption(base_sky, get_zenith_density(lp.y + MULTISCATTER_PHASE));
    vec3 sky = base_sky * zenith * rayleigh_mult;
    vec3 sun = sky_absorption * get_sun_point(p, lp);

    vec3 result = sun + mix(sky * sky_absorption, sky / (sky + 0.5), sun_point_dist_mult);
    return result * 0.5 * (sun_absorption + length(sun_absorption));
}

vec3 jodie_reinhard_tonemap(vec3 color)
{
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 dimmer = color / (color + 1.0);
    return mix(color / (luminance + 1.0), dimmer, dimmer);
}

void main(void)
{
    vec3 color = scatter(normalize(vs_Direction), u_SunDirection);
    color = jodie_reinhard_tonemap(color);
    color = pow(color, vec3(2.2));

    //vec3 sky_color = vec3(0.0, 0.0, 0.0);
    //sky_color += vec3(1.0, 0.0, 0.0);
    //sky_color -= cam_dir.y * cam_dir.y * 0.5;
    //sky_color = mix(sky_color, vec3(0.0), pow(1.0 - max(cam_dir.y, 0.0), 4.0));

    frag_Target.xyz = color;
    frag_Target.w = 1.0;

/*
from iq's shader, https://www.shadertoy.com/view/MdX3Rr
    vec3 lightDir = normalize(vec3(-.8, .15, -.3));
    float sundot = clamp(dot(rd, lightDir), 0.0, 1.0);

    //vec3 skyCol = vec3(.6, .71, .85) - rd.y * .2 * vec3(1., .5, 1.) + .15 * .5;
    vec3 skyCol = vec3(0.3,0.5,0.85) - rd.y*rd.y*0.5;
    skyCol = mix( skyCol, 0.85 * vec3(0.7,0.75,0.85), pow( 1.0 - max(rd.y, 0.0), 4.0 ) );
    
    // sun
    vec3 sun = 0.25 * vec3(1.0,0.7,0.4) * pow( sundot,5.0 );
    sun += 0.25 * vec3(1.0,0.8,0.6) * pow( sundot,64.0 );
    sun += 0.2 * vec3(1.0,0.8,0.6) * pow( sundot,512.0 );
    skyCol += sun;
    
    // horizon
    skyCol = mix( skyCol, 0.68 * vec3(.418, .394, .372), pow( 1.0 - max(rd.y, 0.0), 16.0 ) );
    
    return skyCol;

vec3 getAtmosphericScattering(vec2 p, vec2 lp){
	vec2 correctedLp = lp / max(iResolution.x, iResolution.y) * iResolution.xy;
		
	float zenith = zenithDensity(p.y);
	float sunPointDistMult =  clamp(length(max(correctedLp.y + multiScatterPhase - zenithOffset, 0.0)), 0.0, 1.0);
	
	float rayleighMult = getRayleigMultiplier(p, correctedLp);
	
	vec3 absorption = getSkyAbsorption(skyColor, zenith);
    vec3 sunAbsorption = getSkyAbsorption(skyColor, zenithDensity(correctedLp.y + multiScatterPhase));
	vec3 sky = skyColor * zenith * rayleighMult;
	//vec3 sun = getSunPoint(p, correctedLp) * absorption;
	//vec3 mie = getMie(p, correctedLp) * sunAbsorption;
	
	vec3 totalSky = mix(sky * absorption, sky / (sky + 0.5), sunPointDistMult);
         //totalSky += sun + mie;
	     totalSky *= sunAbsorption * 0.5 + 0.5 * length(sunAbsorption);
	
	return totalSky;
}
    
    // 
*/

    // frag_Target = vec4(Sky(vec3(0.0), normalize(vs_Direction)), 1.0);
    // frag_Target = vec4(0.5 + 0.5 * , 1.0);
    // frag_Target.r = 0.5 + 0.5 * vs_TexCoord.x;
    // frag_Target.g = 1.0;
    // frag_Target.b = 0.5 + 0.5 * vs_TexCoord.y;
    // frag_Target.a = 1.0;
}
