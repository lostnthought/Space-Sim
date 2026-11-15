#version 430 core

/**
 * @file main.vert
 * @brief Projects particles as a circle quad.
 * @ingroup particle_group
 */
// #extension GL_ARB_gpu_shader_int64 : enable
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aVelocity;
out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vVelocity;
out vec3 vViewVelocity;
out vec4 vColor;
out vec2 vTexCoord;
out vec3 vCenter;
out float vSize;
out float vDist;
flat out uint vPID;
// uniform vec3 cam_pos;
uniform vec3 cam_right;
uniform vec3 cam_forward;
uniform vec3 cam_up;

layout(std140, binding = 1) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
};

#define BODY_COUNT 15000
#define MAX_NODES 45000
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


struct MortonEntry
{
    uint pid;  
    uint morton;
    // uint64_t key;
};
layout(std430, binding=2) buffer MortonData {

    MortonEntry entry[BODY_COUNT];
};

struct Node {
    vec4 com_m;
    // min and size
    vec4 min;
    // max and next
    vec3 max;
    int next;
    // left child, right child, parent, depth
    ivec4 lrpd;
    // ivec4 next;
};
#define MAX_DEPTH 64
layout(std430, binding=4) buffer NodeData {
    uint ready_queue[MAX_NODES];
    uint children_remaining[MAX_NODES];
    uint depth_offsets[MAX_DEPTH+1];
    uint pad0;
    uint pad1;
    uint pad2;
    vec4 com_m[MAX_NODES];
    vec4 lrsn[MAX_NODES];
    // min and parent
    vec4 nmin[MAX_NODES];
    // max and depth
    vec4 nmax[MAX_NODES];
};
layout(std430, binding=6) buffer NodeCounter {
    uint queue_out;
    int p_dead_cnt;
    int p_spawn_cnt;
    uint star_pcnt;
};
const vec4 colors[3] =
{
    vec4(0.4, 0.6, 0.7, 1.0),
    vec4(0.7, 0.4, 0.5, 1.0),
    vec4(0.2, 0.6, 0.4, 1.0)
};

vec4 ice = vec4(0.2,0.4,1.0, 1.0);
vec4 rock = vec4(0.5,0.6,0.3, 1.0);
vec4 metal = vec4(0.5,0.5,0.6, 1.0);
vec4 gas = vec4(0.9,0.4,0.2, 1.0);


const vec2 quad[4] = vec2[](
    vec2(-1,-1),
    vec2( 1,-1),
    vec2(-1, 1),
    vec2( 1, 1)
);

void main()
{
    uint id = gl_InstanceID;
    Particle e = p[id];

    if (e.p_m.w <= 0 || id == 0) {
        gl_Position = vec4(0, 0, 0, 1);        
        return;
    }
    vPID = id;
    float massRadius = pow(e.p_m.w, 0.5);
    float dist = length(cam_pos.xyz - e.p_m.xyz);

    float scale = 1.0;
    float size = pow(e.p_m.w, 1.0/3.0) * scale;
    vSize = size;
    vec2 uv = quad[gl_VertexID];
    vTexCoord = uv;
    vec3 pos =
        e.p_m.xyz +
        cam_right * uv.x * size +
        cam_up  * uv.y * size;

    vVelocity = e.vel.xyz;
    vec3 viewVel = (view * vec4(vVelocity,0.0)).xyz;
    
    vec4 material = e.mat;
    vec4 color = p_colors[id];
    if (e.p_m.w <= 0.1) color = vec4(0,0,0,0);

    vDist = length(cam_pos.xyz - e.p_m.xyz);
    vColor = color;
    vViewVelocity = viewVel;

    vCenter = (view * vec4(e.p_m.xyz, 1.0)).xyz;
    vWorldPos = e.p_m.xyz;
    gl_Position = projection * view * vec4(pos, 1.0);        
}
