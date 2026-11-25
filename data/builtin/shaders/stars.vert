#version 330 core

layout(location = 0) in vec3 vert_Position;
layout(location = 1) in float vert_Brightness;
layout(location = 2) in vec2 vert_Local;

uniform mat4 u_SkyViewProjection;
uniform float u_StarAngle;
uniform float u_StarTilt;
uniform float u_StarVisibility;

out float vs_Alpha;
out vec2 vs_Local;

void main(void)
{
    float c = cos(u_StarAngle);
    float s = sin(u_StarAngle);

    vec3 position = vert_Position;
    position.x = vert_Position.x * c - vert_Position.y * s;
    position.y = vert_Position.x * s + vert_Position.y * c;

    // The same tilt the sun's path uses, so the whole
    // celestial sphere keeps moving together with it
    float ct = cos(u_StarTilt);
    float st = sin(u_StarTilt);
    position.yz = mat2(ct, st, -st, ct) * position.yz;

    float above_horizon = smoothstep(-0.02, 0.10, normalize(position).y);

    vs_Alpha = u_StarVisibility * vert_Brightness * above_horizon;
    vs_Local = vert_Local;

    gl_Position = u_SkyViewProjection * vec4(position, 1.0);
}
