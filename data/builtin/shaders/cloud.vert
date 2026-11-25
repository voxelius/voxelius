#version 330 core

layout(location = 0) in vec3 vert_Position;
layout(location = 1) in vec3 vert_Normal;
layout(location = 2) in vec4 vert_Color;

uniform mat4 u_CloudViewProjection;
uniform vec3 u_CameraOrigin;
uniform vec2 u_DeckCenter;
uniform vec3 u_WindOffset;
uniform vec3 u_SunDirection;
uniform vec3 u_SunColor;
uniform vec3 u_SkyColor;
uniform vec3 u_HazeColor;
uniform vec2 u_HazeRange;
uniform vec3 u_CameraPosition;
uniform float u_FadeDistance;
uniform vec2 u_BlendRange;
uniform float u_DepthBias;

out vec4 vs_Color;

const float FADE_START = 0.55;

void main(void)
{
    vec3 world = vert_Position + vec3(u_WindOffset.x, 0.0, u_WindOffset.z);
    vec3 local = world - u_CameraOrigin;

    // The fades belong to the window the mesh was built around; measuring
    // them in the wind-shifted position would move them with the drift
    vec2 offset = vert_Position.xz - u_DeckCenter;
    float horizontal = length(offset);

    // The rings tile square windows, so their own edge is the
    // larger axis: a radial measure would open a gap at the corners
    float square = max(abs(offset.x), abs(offset.y));

    float alpha = vert_Color.a;

    if(u_FadeDistance > 0.0) {
        alpha *= 1.0 - smoothstep(FADE_START, 1.0, horizontal / u_FadeDistance);
    }

    if(u_BlendRange.y > 0.0) {
        alpha *= 1.0 - smoothstep(u_BlendRange.x, u_BlendRange.x + u_BlendRange.y, square);
    }

    // The face normal comes from the mesher; the sun and the sky light the
    // deck here, so a moving sun does not need a rebuild. The constant term
    // keeps a storm cloud from going pure black in its own shadow
    vec3 normal = normalize(vert_Normal);
    float sun = max(dot(normal, u_SunDirection), 0.0);
    float wrap = clamp(dot(normal, u_SunDirection) * 0.5 + 0.5, 0.0, 1.0);
    float sky = 0.65 + 0.35 * max(normal.y, 0.0);

    vec3 to_camera = normalize(u_CameraPosition - world);
    float backlit = pow(max(dot(-to_camera, u_SunDirection), 0.0), 6.0);

    vec3 lit = u_SkyColor * sky;
    lit += u_SunColor * (0.80 * pow(sun, 0.9) + 0.20 * wrap * wrap);
    lit += u_SunColor * backlit * 0.85 + vec3(0.05);

    float haze = smoothstep(u_HazeRange.x, u_HazeRange.y, horizontal);
    vec3 color = mix(vert_Color.rgb * lit, u_HazeColor, haze);

    // The compose step expects premultiplied color
    vs_Color = vec4(color * alpha, alpha);

    gl_Position = u_CloudViewProjection * vec4(local, 1.0);

    // Two rings overlap in the band where one hands over to
    // the other; the coarser one steps back so the finer one wins
    gl_Position.z += u_DepthBias * gl_Position.w;
}
