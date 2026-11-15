#version 430 core

/**
 * @file atmosphere.frag
 * @ingroup atmosphere_group
 * @brief Generates the final color for Rayleigh-Mie based on incoming coefficients.
 * @see https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering
 */
out vec4 fragColor;
in vec3 vPos;
in vec3 vNormal;
in vec3 c0;
in vec3 c1;
in vec3 t0;
in vec3 vViewDir;

uniform float uG;
uniform float uG2;


#define MAX_LAYERS 24
struct WarpLayer
{
    vec3 offset0;
    float freq0;
    vec3 offset1;
    float freq1;
    vec3 offset2;
    float freq2;
    float amp;
    float pad0;
    float pad1;
    float pad3;
};

struct PerlinLayer
{
    float freq;
    float amp;
    float pad0;
    float pad1;
};
    
struct FBMLayer
{
    float freq;
    float amp;
    float pad0;
    float pad1;
};
    
layout(std430, binding=13) buffer PlanetData {

    int warp_count;
    int perlin_count;
    int fbm_count;
    int pad0;
    vec4 sun_dir;
    vec4 sun_color;

    WarpLayer warp_layers[MAX_LAYERS];
    PerlinLayer perlin_layers[MAX_LAYERS];
    FBMLayer fbm_layers[MAX_LAYERS];

};


float rayleigh_phase(float fCos, float fCos2)
{
    return (1 + fCos2) * 3.0 / 4.0;
}

float mie_phase(float fCos, float fCos2)
{

    float f = (3 * (1 - uG2)) / (2 * (2 + uG2));
    float i = (1 + fCos2) / pow((1 + uG2 - 2 * uG * fCos), 3.0 / 2.0);
    return f * i;
    
}
uniform float uColor;
void main()
{

    vec3 color = vec3(0,0,0);

    float fCos = 0;
    float len = length(t0);
    if (len != 0) fCos = dot(normalize(-sun_dir.xyz), t0) / len;
    float fCos2 = fCos * fCos;

    color = rayleigh_phase(fCos, fCos2) * c0 + mie_phase(fCos, fCos2) * c1;

    fragColor = vec4(color, 1.0f);
    
}
