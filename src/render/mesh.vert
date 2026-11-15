
#version 460

/**
 * @file mesh.vert
 * @brief Renders the mesh planet surface, pushing out by planet radius.
 * @ingroup mesh_group
 */
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
out vec3 vNormal;
out vec4 FragPos;
out vec4 vColor;
out float vDist;
out vec4 vFragPosLightSpace;
uniform mat4 light_view;
uniform mat4 light_proj;
uniform float uPlanetRadius;
layout(std140) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
};


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



layout(std430, binding=8) buffer MarchingCubesData 
{
    int EDGE_TO_VERTICES[12][2];
    int EDGE_TABLE[256];
    int TRIANGLE_TABLE[256][16];
};
layout(std430, binding=9) buffer DensityData 
{
    float density[];
};
struct Vertex
{
    vec3 pos;
    float pad0;
    vec3 normal;
    float pad1;
    vec3 color;
    float pad2;
};
#define LODMAX 4
#define MAX_CHUNKS 1000
layout(std430, binding=10) buffer DensityVertexData 
{
    int chunk_vertex_offset[MAX_CHUNKS];
    Vertex vertices[];
};
layout(std430, binding=6) buffer NodeCounter {
    uint queue_out;
    int p_dead_cnt;
    int p_spawn_cnt;
    uint star_pcnt;
};

uniform int uType;
// uniform vec3 cam_pos;

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
	// float f = 1.0;
	vec3 shift = vec3(100);
	for (int i = 0; i < NUM_OCTAVES; ++i) {
		v += a * noise(x);
		x = x * 2.0 + shift;
		// f *= 2.5;
		a *= 0.45;
	}
	return v;
}
vec3 unpack_normal(uint p)
{
    float x = float(p & 0xFF) / 255.0;
    float y = float((p >> 8) & 0xFF) / 255.0;

    vec2 f = vec2(x, y) * 2.0 - 1.0;

    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));

    float t = clamp(-n.z, 0.0, 1.0);
    n.xy += vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);

    return normalize(n);
}
uniform int uSelected;
uniform sampler3D perlin_tex;




float density_fn(vec3 p)
{
    
    float d = (p.z);

    vec3 warp = 
        vec3(texture(perlin_tex, p * 0.001).r * 2.0f - 1.0f,
        texture(perlin_tex, p* 0.001 + vec3(1.0, 1.0, 1.0)).r * 2.0f - 1.0f,
        texture(perlin_tex, p* 0.001 + vec3(5.0, 5.0, 5.0)).r * 2.0f - 1.0f);
    p += warp * 2.0f;
    d += (float(texture(perlin_tex, p * 0.001).r) * 2.0f - 1.0f) * 1.55;
    d += (float(texture(perlin_tex, p * 0.0025).r) * 2.0f - 1.0f) * 1.0;
    d += (float(texture(perlin_tex, p * 0.007).r) * 2.0f - 1.0f) * 0.5;

    d += fbm(p * 0.5) * 8;
    return d;
}
void main()
{


    uint id = gl_VertexID;
    Vertex v = vertices[id];
    vec3 pos = v.pos;
    pos.z += uPlanetRadius;

    float height = pos.z;
    float slope = 1.0 - abs(dot(-v.normal, vec3(0,0,1)));


    vec3 base = mix(vec3(0.2,0.35,0.2), vec3(0.4,0.25,0.2), slope);
    base = mix(base, vec3(0.4, 0.4, 0.5), max(pos.z, 0.0) / 3);

    vColor = vec4(base, 1.0);
    FragPos= vec4(pos, 1.0);
    
    vDist = length(pos - cam_pos.xyz);

    vNormal = normalize(-v.normal);



    vFragPosLightSpace = light_proj * light_view * vec4(pos, 1.0); 
    gl_Position = projection * view * vec4(pos , 1.0);        
}
