#version 330 core

in vec4 vs_Color;

out vec4 frag_Target;

void main(void)
{
    // The vertex shader shaded the face and applied the
    // deck's edge fade; the voxels are opaque where the
    // field is dense, so there is nothing to blend per fragment
    frag_Target = vs_Color;
}
