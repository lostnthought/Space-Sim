
#version 430 core

in vec3 vColor;
out vec4 fragColor;
in vec3 vPos;
uniform samplerCube uCubemap;
uniform vec3 uCamPosWorld;
in vec3 vWorldPos;
uniform float uInnerRadius;
uniform float uOuterRadius;
uniform sampler3D fbm_tex;
uniform sampler3D perlin_tex;
uniform sampler3D worley_tex;
uniform float uPlanetRadius;
const int samples = 2;
layout(std140) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
};

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
void main()
{
    vec3 scaled_cam = uCamPosWorld;
    vec3 dir = vWorldPos - cam_pos.xyz;
    float fFar = length(dir);
    dir /= fFar;
    float b = 2.0 * (dot(scaled_cam.xyz, dir));
    float c = dot(scaled_cam.z, scaled_cam.z) - uOuterRadius * uOuterRadius;
    float det = max(0.0, b * b - 4.0 * c);
    float fNear = 0.5 * (-b - sqrt(det));
    fFar = 0.5 * (-b + sqrt(det));

    fNear = max(fNear, 0.0);
    
    vec3 start = scaled_cam.xyz + dir * fNear;
    fFar -= fNear;


    float sample_length = fFar / samples;


    // swap start to end
    start = start + dir * fFar;
    dir = -dir;
    vec3 ray = dir * sample_length;
    vec3 point = start + ray * 0.5;
    vec3 offset = vec3(cam_pos.xy / 10000, 0);
    point += offset;
    
    float diff = uOuterRadius - uInnerRadius;
    vec3 color = vec3(0,0,0);
    float transmittance = 1;
    for (int i = 0; i < samples; i++)
    {

        
        float len = length(point - offset);
        float h = len - uInnerRadius;
        h = h / diff;
        float thickness = smoothstep(0.1, 0.2, h) * (1-smoothstep(0.7, 0.9, h));
        vec4 s = texture(worley_tex, (point * 3 * 0.5 + 0.5)).rgba;
        float f = texture(fbm_tex, (point * 50 * 0.5 + 0.5)).r * 2 - 1;
        float d = 0;
        d += (s.r + s.g * 0.5) * 0.5;
        d += f * 0.2;
        d -= (s.b + s.a) * 0.2;

        d = max(d, 0.01) * thickness;

        float absorb = (exp(-d * (1 / sample_length) * 2));
        color += d * transmittance /* * dot(dir, sun_dir.xyz) */;
        transmittance *= absorb;
        

        
        point += ray;
    }
    color /= samples;

    

    mat3 rotation = mat3(
        vec3(1, 0, 0),
        vec3(0, 0, 1),
        vec3(0, -1, 0)
    );
    vec3 pos = vPos * rotation;


    fragColor = vec4((color.rgb + texture(uCubemap, pos).rgb) / 1.25, 1.0);
}
