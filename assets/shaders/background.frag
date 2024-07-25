#version 330 core

layout(location = 0) out vec4 target;

uniform float u_Time;

in vec2 frag_TexCoord;

void main(void)
{
    target.x = 0.125 + 0.125 * cos(0.25 * u_Time + frag_TexCoord.x * frag_TexCoord.y);
    target.y = 0.125 + 0.125 * sin(0.25 * u_Time + frag_TexCoord.y * frag_TexCoord.y);
    target.z = 0.250;
    target.w = 1.000;
}
