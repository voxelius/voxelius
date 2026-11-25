#version 330 core

in vec3 vs_Color;
in float vs_Align;

out vec4 frag_Target;

uniform vec3 u_SunTint;
uniform float u_SunStrength;

void main(void)
{
    // The tight halo is the one term a straight blend between two vertices
    // cannot carry, so it stays here; everything else arrives interpolated
    float align2 = vs_Align * vs_Align;
    float align4 = align2 * align2;
    float align8 = align4 * align4;
    float align16 = align8 * align8;
    float sun_halo = align16 * align16;

    frag_Target = vec4(vs_Color + u_SunTint * (sun_halo * 0.35 * u_SunStrength), 1.0);
}
