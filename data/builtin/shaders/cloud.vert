#version 330 core

layout(location = 0) in vec3 vert_Position;
layout(location = 1) in vec4 vert_Color;

uniform mat4 u_CloudViewProjection;
uniform vec3 u_CameraOrigin;
uniform vec2 u_DeckCenter;
uniform vec3 u_WindOffset;
uniform vec3 u_CloudTint;
uniform float u_FadeDistance;
uniform vec2 u_BlendRange;
uniform float u_DepthBias;

out vec4 vs_Color;

// Where the deck's edge starts dissolving. The rest of the
// radius fades to nothing, so the deck has no hard rim at any range
const float FADE_START = 0.55;

void main(void)
{
    // The deck is meshed in world space; the wind offset is what moves it between
    // rebuilds, and the camera origin turns it back into the chunk-local space the view matrix works in
    vec3 world = vert_Position + vec3(u_WindOffset.x, 0.0, u_WindOffset.z);
    vec3 local = world - u_CameraOrigin;

    vec2 offset = world.xz - u_DeckCenter;
    float horizontal = length(offset);

    // The rings tile square windows, so their own edge is the larger axis: a radial measure would open a gap at the corners
    float square = max(abs(offset.x), abs(offset.y));

    float alpha = vert_Color.a;

    // Only the ring at the deck's edge fades out; a ring inside it has nothing to dissolve into
    if(u_FadeDistance > 0.0) {
        alpha *= 1.0 - smoothstep(FADE_START, 1.0, horizontal / u_FadeDistance);
    }

    // A ring fades out over its last cells, into the coarser ring that is already drawn behind it
    if(u_BlendRange.y > 0.0) {
        alpha *= 1.0 - smoothstep(u_BlendRange.x, u_BlendRange.x + u_BlendRange.y, square);
    }

    // The atmosphere tints the whole deck with the setting
    // sun; the compose step expects premultiplied colour
    vs_Color = vec4(vert_Color.rgb * u_CloudTint * alpha, alpha);

    gl_Position = u_CloudViewProjection * vec4(local, 1.0);

    // Two rings overlap in the band where one hands over to the other; the coarser one steps back so the finer one wins
    gl_Position.z += u_DepthBias * gl_Position.w;
}
