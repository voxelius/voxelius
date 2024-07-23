#version 330 core

layout(location = 0) out vec4 target;

uniform float u_Time;

in vec2 frag_TexCoord;

void main(void)
{
    target.x = 0.25 + 0.25 * cos(0.25 * u_Time + frag_TexCoord.x * frag_TexCoord.y);
    target.y = 0.25 + 0.25 * sin(0.25 * u_Time + frag_TexCoord.y * frag_TexCoord.y);
    target.z = 0.50;
    target.w = 1.00;
}
