#version 330 core

#define ANIMATED_BIT 0x00000004U

layout(location = 0) in uint vert_Data1;
layout(location = 1) in uint vert_Data2;
layout(location = 2) in uint vert_Data3;
layout(location = 3) in uint vert_Data4;
layout(location = 4) in uint vert_Data5;

out vec2 vs_TexCoord;
flat out uint vs_FrameIndex;
flat out uint vs_MaskFrame;
flat out vec3 vs_TintColor;
out float vs_Shade;
out float vs_AO;
out float vs_SkyLight;
out float vs_BlockLight;
out vec3 vs_ViewPos;

uniform usamplerBuffer u_AtlasStrips;
uniform samplerBuffer u_ChunkPositions;
uniform mat4 u_ViewProjection;
uniform uint u_AnimationTimer;
uniform vec3 u_CameraLocal;

vec3 unpack_position(uint data_1)
{
    int xpos = int(data_1 & 0x3FFU) - 16;
    int ypos = int((data_1 >> 10U) & 0x3FFU) - 16;
    int zpos = int((data_1 >> 20U) & 0x3FFU) - 16;
    return vec3(xpos, ypos, zpos) / 16.0;
}

vec2 unpack_texcoord(uint data_2)
{
    float u = float(data_2 & 0xFFU) / 255.0;
    float v = float((data_2 >> 8U) & 0xFFU) / 255.0;
    return vec2(u, v);
}

uint unpack_mask_frame(uint data_2)
{
    return (data_2 >> 16U) & 0xFFFFU;
}

uint unpack_albedo_strip(uint data_3)
{
    return data_3 & 0xFFFFU;
}

uint unpack_frame_offset(uint data_3)
{
    return (data_3 >> 16U) & 0xFFFFU;
}

vec3 unpack_tint(uint data_4)
{
    uint tint_bits = (data_4 >> 16U) & 0xFFFFU;
    float r = float(tint_bits & 0x1FU) / 31.0;
    float g = float((tint_bits >> 5U) & 0x3FU) / 63.0;
    float b = float((tint_bits >> 11U) & 0x1FU) / 31.0;
    return vec3(r, g, b);
}

float unpack_shade(uint data_4)
{
    return float((data_4 >> 11U) & 0x1FU) / 31.0;
}

float unpack_sky_light(uint data_4)
{
    return float((data_4 >> 3U) & 0x0FU) / 15.0;
}

float unpack_block_light(uint data_4)
{
    return float((data_4 >> 7U) & 0x0FU) / 15.0;
}

float unpack_ao(uint data_4)
{
    return mix(0.25, 1.0, float(data_4 & 0x03U) / 3.0);
}

bool unpack_animated(uint data_4)
{
    return bool(data_4 & ANIMATED_BIT);
}

void main(void)
{
    vs_TexCoord = unpack_texcoord(vert_Data2);
    vs_MaskFrame = unpack_mask_frame(vert_Data2);

    uint albedo_strip = unpack_albedo_strip(vert_Data3);
    uint frame_offset = unpack_frame_offset(vert_Data3);
    
    uvec4 strip_data = texelFetch(u_AtlasStrips, int(albedo_strip));
    uint strip_frame_base = strip_data.x;
    uint strip_frame_count = strip_data.y;

    gl_Position.w = 1.0;
    gl_Position.xyz = unpack_position(vert_Data1);
    gl_Position.xyz += texelFetch(u_ChunkPositions, int(vert_Data5)).xyz;

    vs_ViewPos = gl_Position.xyz - u_CameraLocal;

    gl_Position = u_ViewProjection * gl_Position;

    if(unpack_animated(vert_Data4)) {
        vs_FrameIndex = strip_frame_base + u_AnimationTimer % max(strip_frame_count, 1U);
    }
    else {
        vs_FrameIndex = strip_frame_base + frame_offset;
    }

    vs_TintColor = unpack_tint(vert_Data4);
    vs_Shade = unpack_shade(vert_Data4);
    vs_AO = unpack_ao(vert_Data4);
    vs_SkyLight = unpack_sky_light(vert_Data4);
    vs_BlockLight = unpack_block_light(vert_Data4);
}
