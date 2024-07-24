#version 330 core

layout(location = 0) out vec4 out_Target;

uniform sampler2DArray u_Textures;

in vec3 frag_TexCoord;
in float frag_Shade;

void main(void)
{
    out_Target = frag_Shade * texture(u_Textures, frag_TexCoord);
}
