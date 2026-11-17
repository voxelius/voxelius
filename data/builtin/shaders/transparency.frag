#version 330 core

in vec2 vs_TexCoord;

out vec4 frag_Target;

uniform sampler2D u_DiffuseColor;
uniform sampler2D u_DiffuseDepth;

uniform sampler2D u_AlphaColor;
uniform sampler2D u_AlphaDepth;

uniform sampler2D u_FluidColor;
uniform sampler2D u_FluidDepth;

uniform sampler2D u_CloudColor;

// Layers whose colour and depth both matter: the opaque world,
// translucent blocks and fluids. Whichever ones touched a pixel
// are sorted by depth and blended from the farthest to the nearest
const int MAX_LAYERS = 3;

// The compose target is 8-bit, so this is where a slow
// gradient becomes bands, and where each channel rounds at its own
// step. In a dark deck over a fog that reads as green and
// magenta stripes instead of a brightness ramp. Half a level
// of noise scatters the steps before the store quantises them
float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);

    return fract((p3.x + p3.y) * p3.z);
}

void main(void)
{
    vec4 colors[MAX_LAYERS];
    float depths[MAX_LAYERS];

    colors[0] = vec4(texture(u_DiffuseColor, vs_TexCoord).rgb, 1.0);
    depths[0] = texture(u_DiffuseDepth, vs_TexCoord).r;

    int count = 1;

    vec4 alpha = texture(u_AlphaColor, vs_TexCoord);

    if(alpha.a > 0.0) {
        colors[count] = alpha;
        depths[count] = texture(u_AlphaDepth, vs_TexCoord).r;
        count += 1;
    }

    vec4 fluid = texture(u_FluidColor, vs_TexCoord);

    if(fluid.a > 0.0) {
        colors[count] = fluid;
        depths[count] = texture(u_FluidDepth, vs_TexCoord).r;
        count += 1;
    }

    // Insertion sort, farthest depth first
    for(int i = 1; i < count; ++i) {
        vec4 color = colors[i];
        float depth = depths[i];
        int j = i - 1;

        while(j >= 0 && depths[j] < depth) {
            colors[j + 1] = colors[j];
            depths[j + 1] = depths[j];
            j -= 1;
        }

        colors[j + 1] = color;
        depths[j + 1] = depth;
    }

    vec3 accum = colors[0].rgb;

    for(int i = 1; i < count; ++i) {
        accum = accum * (1.0 - colors[i].a) + colors[i].rgb;
    }

    // Clouds go last: the deck is above everything that gets this far, and
    // the layer already tested against the terrain depth when it was drawn
    vec4 clouds = texture(u_CloudColor, vs_TexCoord);
    accum = accum * (1.0 - clouds.a) + clouds.rgb;

    float dither = (hash12(gl_FragCoord.xy) - 0.5) / 255.0;

    frag_Target = vec4(accum + dither, 1.0);
}
