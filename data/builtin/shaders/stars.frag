#version 330 core

in float vs_Alpha;
in vec2 vs_Local;

out vec4 frag_Target;

void main(void)
{
    float falloff = 1.0 - clamp(length(vs_Local), 0.0, 1.0);
    float alpha = vs_Alpha * falloff * falloff;

    frag_Target = vec4(vec3(alpha), alpha);
}
