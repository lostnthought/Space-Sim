#version 430 core
/**
 * @file main.frag
 * @brief Shades particles based on lighting and particle material makeup.
 * @ingroup particle_group
 */
in vec3 vWorldPos;
in vec3 vVelocity;
in vec3 vNormal;
in vec3 vViewVelocity;
in vec4 vColor;
in vec2 vTexCoord;
in float vSize;
in float vDist;
in vec3 vCenter;
flat in uint vPID;

layout(location = 0)out vec4 fragColor;
layout(location = 1)out int idColor;

uniform sampler2D uSphere;
uniform samplerCube uSky;
// uniform vec3 cam_pos;
uniform vec3 cam_right;
uniform vec3 cam_forward;
uniform vec3 cam_up;
vec3 blue = vec3(0.4, 0.6, 0.7);
vec4 white = vec4(1.0, 1.0, 1.0, 1.0);
#define BODY_COUNT 15000
#define MAX_NODES 45000
layout(std140, binding = 1) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
};
layout(std140, binding = 2) uniform RenderingData {
    ivec4 resolution;
};
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

uniform sampler3D perlin_tex;
uniform sampler3D fbm_tex;

#define NUM_OCTAVES 5

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}


float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	float f = 1.0;
	vec3 shift = vec3(100);
	for (int i = 0; i < NUM_OCTAVES; ++i) {
		v += a * noise(x * f);
		x = x * 2.0 + shift;
		f *= 2.5;
		a *= 0.45;
	}
	return v;
}

float linearize(float z)
{
    float near = 0.01;
    float far = 100000.0;
    return (2.0 * near) / (far + near - z * (far - near));
}
void main()
{
    vec2 uv = vTexCoord;
    float d = dot(uv, uv);
    if(d > 1.0) discard;
    float alpha = 1.6 - d;
    alpha = smoothstep(0.0, 1.0, alpha);
    uint ppid = vPID;
    if (length(vColor) == 0) discard;
    float z = sqrt(max(1.0 - d, 0.0001));
    Particle u = p[ppid];
    vec3 center = u.p_m.xyz;
    vec3 pos = vWorldPos;
    float size = vSize;
    vec3 normal = normalize(vec3(uv, z));
    vec3 fragWorldPos = vCenter + normal * size;
    float radius = size;

    vec3 forward = normalize(u.vel.xyz);   
    vec3 right = normalize(cross(vec3(0,0,1), forward));
    vec3 up = normalize(cross(forward, right));

    
    vec3 fragPos =
        center +
        cam_right * uv.x * radius +
        cam_up * uv.y * radius;

    vec3 rayDir = normalize(fragPos - cam_pos.xyz);


    vec3 oc = cam_pos.xyz - center;

    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - radius * radius;

    float h = b*b - c;
    if (h < 0.0) discard;

    float t = -b - sqrt(h);
    t = max(t, 0.1);

    vec3 hit = cam_pos.xyz + rayDir * t;

    vec3 worldPos = normalize(hit - center);
    vec3 nn = 
        vec3(dot(forward, worldPos),
        dot(right, worldPos),
        dot(up, worldPos));

    vec4 color = vColor;
     vec4 adiff = vec4(0.1, 0.1, 0.1, 1);
     vec4 mdiff = vec4(0.0, 0.0, 0.0, 1);
     vec4 sdiff = vec4(0.0, 0.0, 0.0, 1);
     vec3 viewDir = normalize(-fragWorldPos);
     for (int i = 0; i < 1; i++)
     {
         uint pid = star_particles[i];
         Particle e = p[pid];
         if (pid == ppid) {
             float mt = clamp(log(e.p_m.w) * 0.2, 0.0, 1.0);

             vec3 star_color = mix(
                 vec3(1.0, 0.3, 0.2),  
                 vec3(0.6, 0.8, 1.0),  
                 mt
             );
             mdiff.rgb += p_colors[pid].rgb * star_color;
             continue;
         }
         vec3 lightVS = (view * vec4(e.p_m.xyz,1)).xyz;
         vec3 dist = lightVS - fragWorldPos;
         float r2 = dot(dist, dist) + 0.01;
         float attenuation = min(500 / r2, 1.0);
         vec3 L = normalize(dist);
         float diff = max(dot(normal, L), 0.0);
         adiff += p_colors[pid] /* * attenuation */ * 0.05;
         mdiff += p_colors[pid] /* * attenuation */ * diff * 0.9;
     }
     color.rgb *= mdiff.rgb + sdiff.rgb + adiff.rgb;

    int out_pid = int(ppid);

    float depth = linearize(gl_FragCoord.z);
        fragColor = vec4(color.rgb, depth);
}
