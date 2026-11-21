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
uniform sampler2D u_CloudDepth;

uniform vec2 u_CloudRange;
uniform vec2 u_CameraRange;

// Layers whose colour and depth both matter: the opaque world,
// translucent blocks, fluids and the cloud deck. Whichever ones
// touched a pixel are sorted by depth and blended from the farthest to the nearest
const int MAX_LAYERS = 4;

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

// A depth buffer value is non-linear, so two layers
// drawn with different near and far planes only compare
// after both are back to a distance along the view axis
float view_distance(float depth, vec2 range)
{
    // A cleared depth attachment holds nothing, and that
    // has to sort behind every layer rather than at the far plane
    if(depth >= 1.0) {
        return 1.0e6;
    }

    float z_near = range.x;
    float z_far = range.y;

    return (2.0 * z_near * z_far) / (z_far + z_near - (2.0 * depth - 1.0) * (z_far - z_near));
}

void main(void)
{
    // Distances along the view axis, not raw depth
    // values: the cloud layer has its own near and far planes
    vec4 colors[MAX_LAYERS];
    float depths[MAX_LAYERS];

    int count = 0;

    colors[count] = vec4(texture(u_DiffuseColor, vs_TexCoord).rgb, 1.0);
    depths[count] = view_distance(texture(u_DiffuseDepth, vs_TexCoord).r, u_CameraRange);
    count += 1;

    vec4 alpha = texture(u_AlphaColor, vs_TexCoord);

    if(alpha.a > 0.0) {
        colors[count] = alpha;
        depths[count] = view_distance(texture(u_AlphaDepth, vs_TexCoord).r, u_CameraRange);
        count += 1;
    }

    vec4 fluid = texture(u_FluidColor, vs_TexCoord);

    if(fluid.a > 0.0) {
        colors[count] = fluid;
        depths[count] = view_distance(texture(u_FluidDepth, vs_TexCoord).r, u_CameraRange);
        count += 1;
    }

    // The deck is just another layer here, so water
    // in front of it hides it just as the ground does
    vec4 clouds = texture(u_CloudColor, vs_TexCoord);

    if(clouds.a > 0.0) {
        colors[count] = clouds;
        depths[count] = view_distance(texture(u_CloudDepth, vs_TexCoord).r, u_CloudRange);
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

    vec3 accum = vec3(0.0);

    for(int i = 0; i < count; ++i) {
        accum = accum * (1.0 - colors[i].a) + colors[i].rgb;
    }

    float dither = (hash12(gl_FragCoord.xy) - 0.5) / 255.0;

    frag_Target = vec4(accum + dither, 1.0);
}
