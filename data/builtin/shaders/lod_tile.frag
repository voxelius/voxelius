#version 330 core
#pragma variant 0 FOG_MODEL

in vec3 vs_Color;
in vec3 vs_ViewPos;

out vec4 frag_Target;

uniform vec3 u_FogColor;
uniform float u_ViewDistance;
uniform vec3 u_SkyColor;
uniform float u_Ambient;

void main(void)
{
    vec3 color = vs_Color * max(vec3(u_Ambient), u_SkyColor);
    float dist = length(vs_ViewPos);
    float fog = 0.0;

#if FOG_MODEL == 1
    // The near edge of the fade keeps a guard: a tiny LOD range would
    // otherwise divide by zero and paint the whole tile with NaN.
    fog = 1.0 - clamp((u_ViewDistance - dist) / max(u_ViewDistance - 16.0, 1.0), 0.0, 1.0);
#elif FOG_MODEL == 2
    float fogd = 2.0 / u_ViewDistance * dist;
    fog = 1.0 - clamp(exp2(fogd * fogd * -1.442695), 0.0, 1.0);
#endif

#if FOG_MODEL != 0
    // The outer edge dissolves into the horizon, but only while the fog is on:
    // with the fog model off the player asked for a clear world.
    fog = max(fog, smoothstep(u_ViewDistance * 0.75, u_ViewDistance, dist));
#endif

    frag_Target.rgb = mix(color, u_FogColor, fog);
    frag_Target.a = 1.0;
}
