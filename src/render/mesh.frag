#version 430 core
/**
 * @file mesh.frag
 * @brief Renders the mesh planet surface and calculates light.
 * @ingroup mesh_group
 */

in vec4 vColor;
in vec3 vNormal;
in vec4 FragPos;
in float vDist;
in vec4 vFragPosLightSpace;
layout(location = 0)out vec4 fragColor;
layout(std140) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
};
// uniform vec3 cam_pos;


uniform int uSelected;
uniform float uTime;
#define BODY_COUNT 30000
#define MAX_NODES 100000
struct Particle
{
	vec4 p_m;
    vec4 mat;
    vec4 vel;
    vec4 acc;
};
layout(std430, binding=1) buffer ParticleData {

	Particle p_spawn[BODY_COUNT];
	vec4 p_colors[BODY_COUNT];
	uint p_dead[BODY_COUNT];
    uint claimed[BODY_COUNT];
	int nodes_visited[BODY_COUNT];
	uint star_particles[BODY_COUNT];
    Particle p[];
};
layout(std430, binding=6) buffer NodeCounter {
    uint queue_out;
    int p_dead_cnt;
    int p_spawn_cnt;
    uint star_pcnt;
};

#define LODMAX 4
#define MAX_CHUNKS 1000
layout(std430, binding=12) buffer ChunkData {
    float lod_dist[3];
    float cpad0;
    int vertex_counts[];
};

uniform sampler2D normal_map;
uniform sampler2D diffuse_tex;
uniform sampler2D spec_tex;
uniform sampler2D ao_tex;
uniform sampler2D shadow_map;

uniform int uType;
uniform int lod_level;
uniform int cid;
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

// const vec3 light_dir[2] = {vec3(0.1, 0.2, -0.8), vec3(-0.3, 0.2, 0.5)};
// const vec3 light[2] = {vec3(1.0, 0.7, 0.5), vec3(0.4, 0.1, 0.6)};

const mat4 ditherMatrix = mat4(
    0.0, 0.5, 0.125, 0.625,
    0.75, 0.25, 0.875, 0.375,
    0.1875, 0.6875, 0.0625, 0.5625,
    0.9375, 0.4375, 0.8125, 0.3125
);
const int bayer16x16[16][16] = {{0, 128, 32, 160, 8, 136, 40, 168, 2, 130, 34, 162, 10, 138, 42, 170},
{192, 64, 224, 96, 200, 72, 232, 104, 194, 66, 226, 98, 202, 74, 234, 106},
{48, 176, 16, 144, 56, 184, 24, 152, 50, 178, 18, 146, 58, 186, 26, 154},
{240, 112, 208, 80, 248, 120, 216, 88, 242, 114, 210, 82, 250, 122, 218, 90},
{12, 140, 44, 172, 4, 132, 36, 164, 14, 142, 46, 174, 6, 134, 38, 166},
{204, 76, 236, 108, 196, 68, 228, 100, 206, 78, 238, 110, 198, 70, 230, 102},
{60, 188, 28, 156, 52, 180, 20, 148, 62, 190, 30, 158, 54, 182, 22, 150},
{252, 124, 220, 92, 244, 116, 212, 84, 254, 126, 222, 94, 246, 118, 214, 86}, 
{3, 131, 35, 163, 11, 139, 43, 171, 1, 129, 33, 161, 9, 137, 41, 169},
{195, 67, 227, 99, 203, 75, 235, 107, 193, 65, 225, 97, 201, 73, 233, 105},
{51, 179, 19, 147, 59, 187, 27, 155, 49, 177, 17, 145, 57, 185, 25, 153},
{243, 115, 211, 83, 251, 123, 219, 91, 241, 113, 209, 81, 249, 121, 217, 89},
{15, 143, 47, 175, 7, 135, 39, 167, 13, 141, 45, 173, 5, 133, 37, 165},
{207, 79, 239, 111, 199, 71, 231, 103, 205, 77, 237, 109, 197, 69, 229, 101},
{63, 191, 31, 159, 55, 183, 23, 151, 61, 189, 29, 157, 53, 181, 21, 149},
{255, 127, 223, 95, 247, 119, 215, 87, 253, 125, 221, 93, 245, 117, 213, 85}};

// const vec2 screen_res = vec2(1280, 960);
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

uniform float uChunkDist;
const vec3 tX = vec3(0, 0, 1);
const vec3 bX = vec3(0, 1, 0);
const vec3 nXw = vec3(1, 0, 0);

const vec3 tY = vec3(1, 0, 0);
const vec3 bY = vec3(0, 0, 1);
const vec3 nYw = vec3(0, 1, 0);

const vec3 tZ = vec3(1, 0, 0);
const vec3 bZ = vec3(0, 1, 0);
const vec3 nZw = vec3(0, 0, 1);


float shadow_calculation(vec4 pos, float diff)
{
    vec3 proj_coords = pos.xyz / pos.w;
    proj_coords = proj_coords * 0.5 + 0.5;
    float closest_depth = texture(shadow_map, proj_coords.xy).r;
    float current_depth = proj_coords.z;


    float bias = max(0.05f * (diff), 0.005f);
    float shadow = 0.0f;
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
    for (int y = -1; y <= 1; y++)
    {
        
        for (int x = -1; x <= 1; x++)
        {

            float pcfDepth = texture(shadow_map, proj_coords.xy + vec2(x, y) * texel_size).r;

            shadow += (current_depth - bias > pcfDepth ? 1.0 : 0.0); 
        }
    }
    // float shadow = (current_depth - bias > closest_depth ? 1.0 : 0.0); 
    shadow /= 9;
    if (proj_coords.z > 1.0) shadow = 0.0;
    
    return shadow;
}

float linearize(float z)
{
    float near = 0.01;
    float far = 100000.0;
    return (2.0 * near) / (far + near - z * (far - near));
}
layout(std140, binding = 2) uniform RenderingData {
    ivec4 resolution;
};
void main()
{

    


    int lod = lod_level;
    float dist = vDist;

    
    float quantized_dist = floor(dist / 20);

    float debug_color = quantized_dist / 10;

    float bias = 0.05;



    float s = sin(uTime * 0.01);
    float c = cos(uTime * 0.01);

    vec3 n = normalize(vec3(vNormal.x, vNormal.y, vNormal.z));
    vec3 adiff;
    vec3 blending = abs(n);
    blending /= (blending.x + blending.y + blending.z);
    float scale = 0.1;
    vec2 uvX = FragPos.yz * scale;
    vec2 uvY = FragPos.xz * scale;
    vec2 uvZ = FragPos.xy * scale;
    vec3 texX = texture(diffuse_tex, uvX).rgb;
    vec3 texY = texture(diffuse_tex, uvY).rgb;
    vec3 texZ = texture(diffuse_tex, uvZ).rgb;

    adiff = texX * blending.x +
                  texY * blending.y +
                  texZ * blending.z;
    vec3 viewDir = normalize(FragPos.xyz - cam_pos.xyz);


    vec3 color = vec3(0.1);
    vec3 L = normalize(sun_dir.xyz);
    float diffuse = max(dot(n, L), 0.0);
    float shadow = shadow_calculation(vFragPosLightSpace, diffuse);
            
        vec3 halfway_dir = normalize(viewDir + L);
        color += diffuse * sun_color.rgb * 2 * (1 - shadow) + sun_color.rgb * 0.3;
    color *= adiff;
    fragColor = vec4(color.rgb, linearize(gl_FragCoord.z));
}
