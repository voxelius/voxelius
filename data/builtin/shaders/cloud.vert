#version 330 core

layout(location = 0) in vec3 vert_Position;
layout(location = 1) in vec4 vert_Color;

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraOrigin;
uniform vec3 u_CameraPos;
uniform vec3 u_WindOffset;
uniform float u_FadeDistance;

out vec4 vs_Color;

// Where the deck's edge starts dissolving. The rest of the
// radius fades to nothing, so the deck has no hard rim at any range
const float FADE_START = 0.55;

void main(void)
{
    // The deck is meshed in wind space; the
    // wind offset is what moves it between rebuilds
    vec3 local = vert_Position + vec3(u_WindOffset.x, 0.0, u_WindOffset.z);

    vec3 world = local + u_CameraOrigin;
    float horizontal = length(world.xz - u_CameraPos.xz);
    float fade = smoothstep(FADE_START, 1.0, horizontal / max(u_FadeDistance, 1.0));

    float alpha = vert_Color.a * (1.0 - fade);

    // The compose step expects premultiplied colour
    vs_Color = vec4(vert_Color.rgb * alpha, alpha);

    gl_Position = u_ViewProjection * vec4(local, 1.0);
}
