#version 330 core

layout(location = 0) in vec3 vert_Position;
layout(location = 1) in vec2 vert_UV;
layout(location = 2) in vec4 vert_Color;
layout(location = 3) in vec2 vert_Light;

out vec2 vs_UV;
out vec4 vs_Color;
out vec2 vs_Light;
out vec3 vs_ViewPos;

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraLocal;

void main(void)
{
    gl_Position = u_ViewProjection * vec4(vert_Position, 1.0);
    vs_ViewPos = vert_Position - u_CameraLocal;
    vs_UV = vert_UV;
    vs_Color = vert_Color;
    vs_Light = vert_Light;
}
