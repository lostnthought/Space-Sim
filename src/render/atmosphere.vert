#version 430 core

/**
 * @file atmosphere.vert
 * @ingroup atmosphere_group
 * @brief Raymarches Rayleigh-Mie through the atmosphere against a sphere.
 * @see https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering
 */

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

out vec3 vPos;
out vec3 vNormal;
out vec3 c0;
out vec3 c1;
out vec3 t0;
out vec3 vViewDir;

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

uniform vec3 uCamPosWorld;
uniform float uInnerRadius;
uniform float uInnerRadius2;
uniform float uOuterRadius;
uniform float uOuterRadius2;
uniform vec3 uInvWavelength;
uniform float uFScale;
uniform float uFScaleOverDepth;
uniform float uFInvScaleDepth;
uniform float uFScaleDepth;
uniform float uG;
uniform float uG2;
uniform float ufKrESun;
uniform float ufKmESun;
uniform float ufKr4Pi;
uniform float ufKm4Pi;
uniform mat4 uFakeView;
uniform mat4 uFakeProj;


float scale(float fCos)
{
    float x = 1.0 - fCos;
    return 0.25 * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

uniform vec3 uCamPos;

const int samples = 4;
void main()
{
    
    vec3 planet_center = vec3(0.0);
    float pscale = 1 / 10000;
    vec3 scaled_cam = uCamPosWorld;
    vec3 apos = aPosition;
    vec3 pos = (aPosition);
    pos *= uOuterRadius;

    vec3 L = normalize(-sun_dir.xyz);
    vec3 sphere_ray_to_cam = pos - scaled_cam.xyz;
    float fFar = length(sphere_ray_to_cam);
    sphere_ray_to_cam /= fFar;
    float b = 2.0 * (dot(scaled_cam.xyz, sphere_ray_to_cam));
    float c = dot(scaled_cam.xyz, scaled_cam.xyz) - uOuterRadius * uOuterRadius;
    float det = max(0.0, b * b - 4.0 * c);
    
    float fNear = 0.5 * (-b - sqrt(det));
    fFar = 0.5 * (-b + sqrt(det));

    fNear = max(fNear, 0.0);
    
    vec3 start = scaled_cam.xyz + sphere_ray_to_cam * fNear;
    fFar -= fNear;

    float start_height = length(start);
    float start_depth = exp(uFScaleOverDepth * (uInnerRadius - start_height));
    float start_angle = dot(sphere_ray_to_cam, start) / start_height;
    float start_offset = start_depth * scale(start_angle);

    float sample_length = fFar / samples;
    float scaled_length = sample_length * uFScale;
    vec3 sample_ray = sphere_ray_to_cam * sample_length;
    vec3 sample_point = start + sample_ray * 0.5;

    vec3 color = vec3(0, 0, 0);

    for (int i = 0; i < samples; i++)
    {

        float height = length(sample_point);
        float depth = exp(uFScaleOverDepth * (uInnerRadius - height));
        float light_angle = dot(L, sample_point) / height;
        float camera_angle = dot(sphere_ray_to_cam, sample_point) / height;
        float scatter = (start_offset + depth * (scale(light_angle) - scale(camera_angle)));
        vec3 att = exp(-scatter * (uInvWavelength * ufKr4Pi + ufKm4Pi));
        color += att * (depth * scaled_length);
        sample_point += sample_ray;
    }
    
    vPos = aPosition;
    vNormal = aNormal;
    c0 = color * (uInvWavelength * ufKrESun);
    c1 = color * ufKmESun;
    t0 = (-sphere_ray_to_cam);

    vViewDir = normalize(vec3(aPosition - cam_pos.xyz));
    vec3 p2 = pos;
    p2.z -= uInnerRadius;
    p2.z -= scaled_cam.z - uInnerRadius;
    vec4 worldPos = uFakeProj * uFakeView * vec4(p2+ cam_pos.xyz, 1.0);
    gl_Position = worldPos;

}
