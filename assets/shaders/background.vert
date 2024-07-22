#version 330 core

layout(location = 0) in vec2 vert_Position;

out vec2 frag_TexCoord;

void main(void)
{
    frag_TexCoord = 0.5 + 0.5 * vert_Position;
    gl_Position.xy = vert_Position;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
}
