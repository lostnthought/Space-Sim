#version 440 
layout(location = 0) in vec3 aPosition;

/**
 * @file water.vert
 * @ingroup water_group
 * @brief The shader that handles water vertex interpolation and normal sampling.
 */
layout(std140) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
};

out vec3 vNormal;
out vec3 vPos;
out vec3 rvPos;
out vec3 rvNormal;
out float vDist;
out vec2 vUV;
out vec3 vViewDir;
out vec4 vFragPosLightSpace;


const float water_scale = 256;
const float water_level = -3.0f;

uniform float uTime;
uniform sampler2D uFFT;
uniform sampler2D uSlope;
uniform sampler2D shadow_map;
uniform float uPlanetRadius;
uniform mat4 light_proj;
uniform mat4 light_view;

int L = 1000;
int N = 128;
int NUM_TILES_PER_WATER_SCALE = 1;


void main()
{
    vec3 pos = aPosition;
    float patchSize = water_scale;

    pos.xy = aPosition.xy * water_scale + cam_pos.xy;
    vec2 uv = ((aPosition.xy * water_scale + cam_pos.xy) / (N / NUM_TILES_PER_WATER_SCALE));

    vec2 texel = vec2(1.0 / N);

    float h1 = texture(uFFT, uv).r;
    vec2 dxy1 = texture(uSlope, uv).rb;

    float strength = 1.0 / 0.15;

    float dx = dxy1.x * strength;
    float dy = dxy1.y * strength;
    float h = h1;

    pos.z = water_level + h;    
    pos.z += uPlanetRadius;
    vec2 grad = vec2(dx, dy);
    pos.xy += grad;

    vec3 normal = normalize(vec3(dx, dy, 1.0));
    
    vNormal = normal;

    vUV = uv;
    vPos = pos;

    vDist = h;

    vViewDir = normalize(cam_pos.xyz - pos.xyz);
    
    vFragPosLightSpace = light_proj * light_view * vec4(pos, 1.0); 
    gl_Position = projection * view * vec4(pos, 1.0);        
    
}
