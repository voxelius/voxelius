#version 330 core
#pragma variant 0 FOG_MODEL

in vec2 vs_TexCoord;
flat in uint vs_FrameIndex;
flat in uint vs_MaskFrame;
flat in vec3 vs_TintColor;
in float vs_Shade;
in float vs_AO;
in float vs_SkyLight;
in float vs_BlockLight;
in vec3 vs_ViewPos;

out vec4 frag_Target;

uniform vec3 u_FogColor;
uniform float u_ViewDistance;
uniform vec3 u_SkyColor;
uniform vec3 u_BlockColor;
uniform float u_Ambient;
uniform sampler2DArray u_AtlasTexture;
uniform samplerBuffer u_AtlasFrames;

struct AtlasFrame {
    vec2 uv_min;
    vec2 uv_max;
    uint layer;
};

AtlasFrame get_frame(int index)
{
    vec4 part_1 = texelFetch(u_AtlasFrames, index * 2);
    vec4 part_2 = texelFetch(u_AtlasFrames, index * 2 + 1);

    AtlasFrame frame;
    frame.uv_min = part_1.xy;
    frame.uv_max = part_1.zw;
    frame.layer = floatBitsToUint(part_2.x);
    
    return frame;
}

void main(void)
{
    AtlasFrame albedo_frame = get_frame(int(vs_FrameIndex));
    vec2 albedo_uv = mix(albedo_frame.uv_min, albedo_frame.uv_max, vs_TexCoord);
    vec4 albedo_color = texture(u_AtlasTexture, vec3(albedo_uv, float(albedo_frame.layer)));

    AtlasFrame mask_frame = get_frame(int(vs_MaskFrame));
    vec2 mask_uv = mix(mask_frame.uv_min, mask_frame.uv_max, vs_TexCoord);
    vec4 mask_color = texture(u_AtlasTexture, vec3(mask_uv, float(mask_frame.layer)));

    vec3 tint = mix(vec3(1.0), vs_TintColor, mask_color.r);
    vec3 light = max(vec3(u_Ambient), max(u_SkyColor * vs_SkyLight, u_BlockColor * vs_BlockLight));
    vec3 shaded = albedo_color.rgb * vs_Shade * vs_AO * tint * light;

    frag_Target.rgb = shaded;
    frag_Target.a = albedo_color.a;

#if FOG_MODEL == 1
    float dist = length(vs_ViewPos);
    float fog_factor = 1.0 - clamp((u_ViewDistance - dist) / (u_ViewDistance - 16.0), 0.0, 1.0);
    frag_Target.rgb = mix(frag_Target.rgb, u_FogColor, fog_factor);
#elif FOG_MODEL == 2
    float dist = length(vs_ViewPos);
    float fogd = 2.0 / u_ViewDistance * dist;
    float fog_factor = 1.0 - clamp(exp2(fogd * fogd * -1.442695), 0.0, 1.0);
    frag_Target.rgb = mix(frag_Target.rgb, u_FogColor, fog_factor);
#endif

    frag_Target.rgb *= frag_Target.a;
}
