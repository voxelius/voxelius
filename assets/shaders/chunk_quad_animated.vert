// ANIMATED
#version 330 core

#define FACING_NORTH    0x0000U
#define FACING_SOUTH    0x0001U
#define FACING_EAST     0x0002U
#define FACING_WEST     0x0003U
#define FACING_UP       0x0004U
#define FACING_DOWN     0x0005U
#define FACING_NESW     0x0006U
#define FACING_NWSE     0x0007U

layout(location = 0) in vec3 vert_Position;
layout(location = 1) in uvec2 vert_Quad;

// FIXME: add variant shaders
layout(packed) uniform u_Animations {
    uint animations[512];
};

uniform mat4x4 u_CameraMatrix;
uniform vec3 u_WorldPosition;
uniform uvec3 u_Timings;

out vec3 frag_TexCoord;
out float frag_Shade;

void main(void)
{
    vec3 quad_offset;
    quad_offset.x = float(0x00FFU & (vert_Quad.x >> 24U)) / 16.0;
    quad_offset.y = float(0x00FFU & (vert_Quad.x >> 16U)) / 16.0;
    quad_offset.z = float(0x00FFU & (vert_Quad.x >> 8U))  / 16.0;

    vec2 quad_scale;
    quad_scale.x = float((0x000FU & (vert_Quad.x >> 4U)) + 1U) / 16.0;
    quad_scale.y = float((0x000FU & (vert_Quad.x >> 0U)) + 1U) / 16.0;

    uint quad_facing = (0x000FU & (vert_Quad.y >> 28U));
    uint quad_toffset = (0x07FFU & (vert_Quad.y >> 17));
    uint quad_tframes = (0x001FU & (vert_Quad.y >> 12)) + 1U;

    gl_Position.xyz = vert_Position;
    gl_Position.x *= quad_scale.x;
    gl_Position.z *= quad_scale.y;

    if(quad_facing == FACING_NORTH) {
        gl_Position.xyz = vec3(gl_Position.x, gl_Position.z + 1.0, -1.0);
        frag_TexCoord.xy = vec2(1.0 - vert_Position.x, vert_Position.z + 1.0);
        frag_Shade = 0.8;
    }
    else if(quad_facing == FACING_SOUTH) {
        gl_Position.xyz = vec3(gl_Position.x, -gl_Position.z, 0.0);
        frag_TexCoord.xy = vec2(vert_Position.x, -vert_Position.z);
        frag_Shade = 0.8;
    }
    else if(quad_facing == FACING_EAST) {
        gl_Position.xyz = vec3(1.0, 1.0 - gl_Position.x, gl_Position.z);
        frag_TexCoord.xy = vec2(-vert_Position.z, 1.0 - vert_Position.x);
        frag_Shade = 0.6;
    }
    else if(quad_facing == FACING_WEST) {
        gl_Position.xyz = vec3(0.0, gl_Position.x, gl_Position.z);
        frag_TexCoord.xy = vec2(1.0 + vert_Position.z, vert_Position.x);
        frag_Shade = 0.6;
    }
    else if(quad_facing == FACING_UP) {
        gl_Position.xyz = vec3(gl_Position.x, 1.0, gl_Position.z);
        frag_TexCoord.xy = vec2(vert_Position.x, -vert_Position.z);
        frag_Shade = 1.0;
    }
    else if(quad_facing == FACING_DOWN) {
        gl_Position.xyz = vec3(gl_Position.x, 0.0, -1.0 - gl_Position.z);
        frag_TexCoord.xy = vec2(vert_Position.x, -vert_Position.z);
        frag_Shade = 0.4;
    }
    else {
        gl_Position.xyz = vec3(gl_Position.x, 0.0, gl_Position.z);
        frag_TexCoord.xy = vec2(vert_Position.x, -vert_Position.z);
        frag_Shade = 1.0;
    }
    
    // FIXME: add variant shaders
    frag_TexCoord.z = floor(float(animations[quad_toffset + u_Timings.z % quad_tframes]) + 0.5);

    gl_Position.w = 1.0;
    gl_Position.xyz += quad_offset + u_WorldPosition;
    gl_Position = u_CameraMatrix * gl_Position;
}
