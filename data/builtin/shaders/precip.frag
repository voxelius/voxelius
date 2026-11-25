#version 330 core

in vec2 vs_UV;
in vec4 vs_Color;
in vec2 vs_Light;
in vec3 vs_ViewPos;

out vec4 frag_Target;

uniform sampler2D u_Texture;
uniform vec3 u_FogColor;
uniform float u_ViewDistance;
uniform vec3 u_SkyColor;
uniform vec3 u_BlockColor;
uniform float u_Ambient;

void main(void)
{
    vec4 tex = texture(u_Texture, vs_UV);
    float alpha = tex.a * vs_Color.a;

    if(alpha < 0.01) {
        discard;
    }

    vec3 lit = max(vec3(u_Ambient), max(u_SkyColor * vs_Light.x, u_BlockColor * vs_Light.y));

    // Distant precipitation takes the
    // color of the air in front of it
    vec3 color = tex.rgb * vs_Color.rgb * lit;
    float fog = smoothstep(u_ViewDistance * 0.35, u_ViewDistance, length(vs_ViewPos));
    color = mix(color, u_FogColor, clamp(fog, 0.0, 1.0));
    frag_Target = vec4(color * alpha, alpha);
}
