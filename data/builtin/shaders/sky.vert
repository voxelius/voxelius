#version 330 core

out vec2 vs_Ndc;

void main(void)
{
    gl_Position.x = -1.0 + float((gl_VertexID & 1) << 2);
    gl_Position.y = -1.0 + float((gl_VertexID & 2) << 1);
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;

    vs_Ndc = gl_Position.xy;
}
