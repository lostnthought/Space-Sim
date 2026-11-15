#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform float uInnerRadius;
uniform float uOuterRadius;
uniform vec3 uCamPosWorld;
uniform sampler3D fbm_tex;
uniform sampler3D perlin_tex;
uniform sampler3D worley_tex;
uniform mat4 uFakeProj;
uniform mat4 uFakeView;
out vec3 vPos;
out vec3 vWorldPos;
out vec3 vColor;

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
const int samples = 4;
float scale(float fCos)
{
    float x = 1.0 - fCos;
    return 0.25 * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}
void main()
{

    vec3 scaled_cam = uCamPosWorld;
    float uFScale = 1 / (0.01);
    float uFScaleDepth = 0.25;
    float uFScaleOverDepth = uFScale / uFScaleDepth;
    vec3 pos = normalize(aPosition);
    pos *= uOuterRadius;
    vWorldPos = pos + cam_pos.xyz;
    vec3 dir = pos;
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
    vColor = color;

    vPos = normalize(aPosition);
    gl_Position = uFakeProj * uFakeView * vec4(pos + cam_pos.xyz, 1.0);

}
