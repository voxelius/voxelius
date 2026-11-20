#version 330 core

in vec2 vs_Ndc;

out vec4 frag_Target;

uniform vec3 u_CameraForward;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;
uniform float u_TanHalfFov;
uniform float u_Aspect;

uniform vec3 u_HorizonColor;
uniform vec3 u_SunHorizonColor;
uniform vec3 u_ZenithColor;
uniform vec3 u_GroundColor;
uniform vec3 u_SunDirection;
uniform vec3 u_SunTint;
uniform float u_SunStrength;
uniform float u_Twilight;

void main(void)
{
    vec3 ray = u_CameraForward;
    ray += u_CameraRight * (vs_Ndc.x * u_Aspect * u_TanHalfFov);
    ray += u_CameraUp * (vs_Ndc.y * u_TanHalfFov);

    vec3 direction = normalize(ray);
    float up = clamp(direction.y, 0.0, 1.0);

    // The warm horizon only builds up on the sun's side; the
    // opposite horizon keeps the neutral colour
    vec2 view_flat = direction.xz;
    vec2 sun_flat = u_SunDirection.xz;
    float view_length = length(view_flat);
    float sun_length = length(sun_flat);
    float side = 0.0;

    if(view_length > 0.0001 && sun_length > 0.0001) {
        side = max(dot(view_flat / view_length, sun_flat / sun_length), 0.0);
    }

    vec3 horizon = mix(u_HorizonColor, u_SunHorizonColor, pow(side, 2.0) * u_Twilight);
    vec3 color = mix(horizon, u_ZenithColor, pow(up, 0.50));

    // Below the horizon the same colour turns into a dark
    // ground, so a hole in the terrain does not show sky
    if(direction.y < 0.0) {
        color = mix(horizon, u_GroundColor, clamp(-direction.y * 4.0, 0.0, 1.0));
    }

    // Rayleigh scattering around the sun, strongest along the horizon
    // where the light crosses the most air and keeps the red end of its
    // spectrum; the broad term is the warm glow that survives the mix
    float align = max(dot(direction, u_SunDirection), 0.0);
    float rayleigh = 0.75 * (1.0 + align * align);
    float horizon_band = pow(1.0 - up, 8.0);
    float sun_halo = pow(align, 32.0);

    color += u_SunTint * (rayleigh * horizon_band * u_Twilight * 0.55);
    color += u_SunTint * (pow(align, 4.0) * u_Twilight * 0.08);
    color += u_SunTint * (sun_halo * 0.35 * u_SunStrength);

    frag_Target = vec4(color, 1.0);
}
