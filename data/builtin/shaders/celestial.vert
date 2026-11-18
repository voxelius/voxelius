#version 330 core

uniform mat4 u_SkyViewProjection;
uniform vec3 u_Direction;
uniform float u_Radius;
uniform float u_HalfSize;
uniform float u_Roll;
uniform vec4 u_UvRect;

out vec2 vs_Uv;

void main(void)
{
    vec2 corner;
    corner.x = (gl_VertexID & 1) == 0 ? -1.0 : 1.0;
    corner.y = (gl_VertexID & 2) == 0 ? -1.0 : 1.0;

    vec3 forward = normalize(u_Direction);
    vec3 helper = abs(forward.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(helper, forward));
    vec3 up = cross(forward, right);

    float c = cos(u_Roll);
    float s = sin(u_Roll);

    vec2 rolled;
    rolled.x = corner.x * c - corner.y * s;
    rolled.y = corner.x * s + corner.y * c;

    vec3 position = forward * u_Radius + (right * rolled.x + up * rolled.y) * u_HalfSize;

    gl_Position = u_SkyViewProjection * vec4(position, 1.0);
    vs_Uv = u_UvRect.xy + (corner * 0.5 + 0.5) * u_UvRect.zw;
}
