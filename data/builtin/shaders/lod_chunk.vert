#version 330 core

layout(location = 0) in uint vert_Cell;
layout(location = 1) in uint vert_Color;
layout(location = 2) in ivec3 vert_Origin;

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraOrigin;
uniform float u_CellSize;
uniform float u_DepthBias;
uniform int u_DebugMode;
uniform vec3 u_DebugColor;

out vec3 vs_Color;
out vec3 vs_ViewPos;

const vec2 CORNERS[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main(void)
{
    uint cell_x = vert_Cell & 0x0FU;
    uint cell_y = (vert_Cell >> 4U) & 0x0FU;
    uint cell_z = (vert_Cell >> 8U) & 0x0FU;
    uint face = (vert_Cell >> 12U) & 0x07U;

    vec3 origin = vec3(vert_Origin);
    vec3 base = origin + vec3(cell_x, cell_y, cell_z) * u_CellSize;

    vec2 corner = CORNERS[gl_VertexID];

    vec3 position;
    float shade;

    if(face == 0U) {
        position = base + vec3(corner.x, 1.0, corner.y) * u_CellSize;
        shade = 1.0;
    }
    else if(face == 1U) {
        position = base + vec3(corner.x, 0.0, corner.y) * u_CellSize;
        shade = 0.4;
    }
    else if(face == 2U) {
        position = base + vec3(corner.x, corner.y, 0.0) * u_CellSize;
        shade = 0.8;
    }
    else if(face == 3U) {
        position = base + vec3(corner.x, corner.y, 1.0) * u_CellSize;
        shade = 0.8;
    }
    else if(face == 4U) {
        position = base + vec3(1.0, corner.y, corner.x) * u_CellSize;
        shade = 0.6;
    }
    else {
        position = base + vec3(0.0, corner.y, corner.x) * u_CellSize;
        shade = 0.6;
    }

    float r = float(vert_Color & 0xFFU) / 255.0;
    float g = float((vert_Color >> 8U) & 0xFFU) / 255.0;
    float b = float((vert_Color >> 16U) & 0xFFU) / 255.0;

    if(u_DebugMode != 0) {
        vs_Color = u_DebugColor;
    }
    else {
        vs_Color = vec3(r, g, b) * shade;
    }

    // A full water block is one sixteenth below its own top, so the far sea
    // has to sit exactly there or the seam against the fluid pass shows.
    if(0U != (vert_Color & (1U << 24U))) {
        position.y -= 0.0625;
    }

    vec3 relative = position - u_CameraOrigin;
    vs_ViewPos = relative;

    gl_Position = u_ViewProjection * vec4(relative, 1.0);
    gl_Position.z += u_DepthBias * gl_Position.w;
}
