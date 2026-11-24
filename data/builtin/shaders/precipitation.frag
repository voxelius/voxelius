#version 330 core

in vec2 vs_UV;
in vec4 vs_Color;
in vec3 vs_ViewPos;

out vec4 frag_Target;

uniform sampler2D u_Texture;
uniform vec3 u_FogColor;
uniform float u_ViewDistance;

void main(void)
{
    vec4 tex = texture(u_Texture, vs_UV);
    float alpha = tex.a * vs_Color.a;

    if(alpha < 0.01) {
        discard;
    }

    // Distant precipitation takes the
    // color of the air in front of it
    vec3 color = tex.rgb * vs_Color.rgb;
    float fog = smoothstep(u_ViewDistance * 0.35, u_ViewDistance, length(vs_ViewPos));
    color = mix(color, u_FogColor, clamp(fog, 0.0, 1.0));
    frag_Target = vec4(color * alpha, alpha);
}
