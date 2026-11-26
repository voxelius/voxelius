#version 330 core

layout(location = 0) in vec3 vert_Direction;

out vec3 vs_Color;
out float vs_Align;

uniform mat4 u_ViewProjection;

uniform vec3 u_HorizonColor;
uniform vec3 u_SunHorizonColor;
uniform vec3 u_ZenithColor;
uniform vec3 u_SunDirection;
uniform vec2 u_SunAzimuth;
uniform vec3 u_SunTint;
uniform float u_Scatter;

void main(void)
{
    gl_Position = u_ViewProjection * vec4(vert_Direction, 1.0);

    float up = clamp(vert_Direction.y, 0.0, 1.0);

    vec2 view_flat = vert_Direction.xz;
    float flat_length2 = dot(view_flat, view_flat);
    float side = 0.0;

    if(flat_length2 > 1.0e-8) {
        side = max(dot(view_flat, u_SunAzimuth) * inversesqrt(flat_length2), 0.0);
    }

    vec3 horizon = mix(u_HorizonColor, u_SunHorizonColor, side * side * u_Scatter);
    vec3 color = mix(horizon, u_ZenithColor, sqrt(up));

    float align = max(dot(vert_Direction, u_SunDirection), 0.0);
    float align2 = align * align;
    float align4 = align2 * align2;

    float band = 1.0 - up;
    float band2 = band * band;
    float band4 = band2 * band2;

    float rayleigh = 0.75 * (1.0 + align2);
    float horizon_band = band4 * band4;

    color += u_SunTint * (rayleigh * horizon_band * u_Scatter * 0.55);
    color += u_SunTint * (align4 * u_Scatter * 0.08);

    vs_Color = color;
    vs_Align = align;
}
