#version 330 core

layout(location = 0) in uint vert_Cell;
layout(location = 1) in uint vert_Height;
layout(location = 2) in uint vert_Color;
layout(location = 3) in ivec2 vert_OriginXZ;

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraOrigin;
uniform float u_CellSize;
uniform float u_DepthBias;
uniform int u_DebugMode;
uniform vec3 u_DebugColor;

out vec3 vs_Color;
out vec3 vs_ViewPos;

const vec2 CORNERS[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

int unpack_height(uint bits)
{
    int value = int(bits & 0xFFFFU);
    return value >= 32768 ? value - 65536 : value;
}

void main(void)
{
    uint cell_x = vert_Cell & 0x1FU;
    uint cell_z = (vert_Cell >> 5U) & 0x1FU;
    uint face = (vert_Cell >> 10U) & 0x07U;
    uint span_x = ((vert_Cell >> 13U) & 0x1FU) + 1U;
    uint span_z = ((vert_Cell >> 18U) & 0x1FU) + 1U;

    int top = unpack_height(vert_Height);
    int bottom = unpack_height(vert_Height >> 16U);

    vec3 origin = vec3(float(vert_OriginXZ.x), 0.0, float(vert_OriginXZ.y));
    vec3 base = origin + vec3(cell_x, 0.0, cell_z) * u_CellSize;

    vec2 corner = CORNERS[gl_VertexID];
    float span = mix(float(bottom), float(top), corner.y);

    vec3 position;
    float shade;

    if(face == 0U) {
        position = base + vec3(corner.x * u_CellSize * float(span_x), float(top), corner.y * u_CellSize * float(span_z));
        shade = 1.0;
    }
    else if(face == 1U) {
        position = base + vec3(corner.x * u_CellSize * float(span_x), float(bottom), corner.y * u_CellSize * float(span_z));
        shade = 0.4;
    }
    else if(face == 2U) {
        position = base + vec3(corner.x * u_CellSize, span, 0.0);
        shade = 0.8;
    }
    else if(face == 3U) {
        position = base + vec3(corner.x * u_CellSize, span, u_CellSize);
        shade = 0.8;
    }
    else if(face == 4U) {
        position = base + vec3(u_CellSize, span, corner.x * u_CellSize);
        shade = 0.6;
    }
    else {
        position = base + vec3(0.0, span, corner.x * u_CellSize);
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
