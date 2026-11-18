#version 330 core

in vec2 vs_Uv;

out vec4 frag_Target;

uniform sampler2D u_Texture;
uniform vec3 u_Tint;
uniform float u_Opacity;

void main(void)
{
    vec4 texel = texture(u_Texture, vs_Uv);
    float alpha = texel.a * u_Opacity;

    // Premultiplied, so the additive sun and the alpha-blended moon
    // can share one program and differ only in the blend function
    frag_Target = vec4(texel.rgb * u_Tint * alpha, alpha);
}
