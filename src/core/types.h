#ifndef TYPES_H

#define TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "nk_config.h"
#include "nuklear/nuklear.h"
#include <omp.h>
#include "nuklear/nuklear_sdl_gl3.h"



/**
 * @defgroup water_group Water
 * @brief All water related rendering and FFT generation.
 */

/**
 * @defgroup atmosphere_group Atmosphere
 * @brief The system for clouds and atmospheric scattering generation and rendering.
 */

/**
 * @defgroup mesh_group Planet Surfaces
 * @brief The system for planet surface generation and rendering.
 */

/**
 * @defgroup particle_group Particles
 * @brief The system for n-body updates and rendering.
 */

#define FOV 90
#define VIEW_NEAR 0.01
#define VIEW_FAR 100000.0

#define BODY_COUNT 15000
#define MAX_NODES 45000

#define SHADOW_MAP_SIZE 2048
#define QUERY_FRAMES 3

typedef struct vec2 {float x, y;} vec2;
typedef struct vec3 {float x, y, z;} vec3;
typedef struct vec4 {float x, y, z, w;} vec4;
typedef struct ivec3 {int x, y, z;} ivec3;
typedef struct ivec4 {int x, y, z, w;} ivec4;
typedef struct uvec4 {uint32_t x, y, z, w;} uvec4;
typedef struct mat4 {float a0, a1, a2, a3, b0, b1, b2, b3, c0, c1, c2, c3, d0, d1, d2, d3;} mat4;

typedef struct {
    uint64_t start;
    uint64_t freq;
} Timer;

static inline void start_timer(Timer* t) {
    t->start = SDL_GetPerformanceCounter();
    t->freq = SDL_GetPerformanceFrequency();
}

static inline double end_timer(Timer* t) {
    Uint64 end = SDL_GetPerformanceCounter();
    return (double)(end - t->start) * 1000.0 / (double)t->freq; 
}


typedef enum PerformanceQueryTypes
{

    PQUERY_MAIN,
    PQUERY_BOUNDS,
    PQUERY_MORTON,
    PQUERY_RADIX,
    PQUERY_TREECOUNT,
    PQUERY_TREE,
    PQUERY_COM,
    PQUERY_ATMOSPHERE,
    PQUERY_CLOUDS,
    PQUERY_WATER_GEN,
    PQUERY_WATER,
    PQUERY_MESH_GEN,
    PQUERY_PARTICLE,
    PQUERY_MESH,
    PQUERY_DRAW,
    PQUERY_POSTPROCESS,
    PQUERY_MAX

} PerformanceQueryTypes;

typedef struct Particle
{
    vec4 p_m;
    vec4 mat;
    vec4 vel;
    vec4 acc;
  
} Particle;

typedef struct ParticleVertex
{
    vec3 pos;
    vec3 vel;
  
} ParticleVertex;

typedef struct Node
{

    vec3 center_of_mass;
    vec3 center;
    float mass;
    int first_child;
    bool subdivided;
    float half_size;
    int body_index;
    
} Node;

typedef struct MortonPair
{
    uint32_t pid;
    uint32_t m;
    
} MortonPair;

typedef struct ParticleData
{
    
    Particle p_spawn[BODY_COUNT];
    vec4 p_colors[BODY_COUNT];
    uint32_t p_dead[BODY_COUNT];
    uint32_t claimed[BODY_COUNT];
    int nodes_visited[BODY_COUNT];
    uint32_t star_particles[BODY_COUNT];
    Particle p[BODY_COUNT];
} ParticleData;
typedef struct MortonData
{
    MortonPair entry[BODY_COUNT];
    
} MortonData;

typedef struct GPUNode {
    
    vec4 com_m;
    // vec4 hs;
    vec4 min;
    vec3 max;
    int next;
    ivec4 lrpd;
} GPUNode;
#define MAX_DEPTH 64

typedef struct NodeData {
    uint32_t ready_queue[MAX_NODES];
    uint32_t children_remaining[MAX_NODES];
    uint32_t depth_offsets[MAX_DEPTH+1];
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;
    vec4 com_m[MAX_NODES];
    vec4 lrsn[MAX_NODES];
    vec4 nmin[MAX_NODES];
    vec4 nmax[MAX_NODES];

    // GPUNode nodes[MAX_NODES];
} NodeData;

#define WORKGROUP_SIZE 128
#define RADIX_BIN_SIZE 128

typedef struct RadixData
{
    uint32_t local_prefix[5000 * WORKGROUP_SIZE];
    uint32_t local_offsets[BODY_COUNT];
    uint32_t block_bin_count[5000 * RADIX_BIN_SIZE];
    uint32_t global_offsets[5000 *RADIX_BIN_SIZE];
    uint32_t local_bin_count[5000 * RADIX_BIN_SIZE];
} RadixData;
#define MAX_CHUNKS 1000

#define MAX_CHUNK_LIST (MAX_CHUNKS * 20)



typedef struct ChunkList
{
    int lod0;
    int lod1;
    int offset0;
    int offset1;
    int id0;
    int id1;
    vec3 center;
    vec3 min;
    vec3 max;
    bool free;
    
}ChunkList;

typedef struct NodeCounter
{
    uint32_t queue_out;
    int p_dead_cnt;
    int p_spawn_cnt;
    uint32_t star_pcnt;
    
} NodeCounter;

typedef struct BoundsData {
    uint32_t minx;
    uint32_t miny;
    uint32_t minz;
    uint32_t maxx;
    uint32_t maxy;
    uint32_t maxz;
} BoundsData;

typedef struct CamData
{
    
    vec4 cam_pos, cam_vel;
    mat4 proj, view, inv_proj, inv_view;
} CamData;
typedef struct Render
{
    ivec4 resolution;
} RenderData;
typedef struct Camera
{
    vec3 forward, right, up, dir, target;
    float yaw, pitch, roll;
    CamData cam_data;

    // float x, y, z;
    // vec3 velocity;

    bool on_ground;
    
} Camera;
typedef struct ChunkScore
{
    int index;
    int lod;
    float score;
    bool active;
    
} ChunkScore;

typedef struct Globals
{

    int hovered_body;
    int selected_body;
    bool lock_on;
    bool paused;
    float min_mass;
    float max_mass;
    float mass_pow;
    float radius;
    bool force_space_cam;
    bool planet_meshing_toggle;
    bool water_toggle;
    bool particle_toggle;
    bool sky_toggle;
    bool profiling;
    
    
} Globals;


// 52^3
// LOD0 = 1
// 
// 24^3
// LOD1 = 27
// 
// 12^3
// LOD2 = VIEW FRUSTUM = 5-6
// 

typedef enum LodLevels
{

    LOD0,
    LOD1,
    LOD2,
    LOD3,
    LODMAX
    
} LodLevels;


// max chunks per lod level
#define MAX_DENSITY_RESOLUTION 64

static const int LOD_RES[LODMAX] =
{
    16, 8, 4, 2
};

// lod0 has slots in lod1, lod1 has slots in lod2
// each chunk gets both emitted always, higher lod parent points to child lower lod in the other array
// this means that each lod level past 0 must contain enough to house the parent

// 8 | 16 + 8 | 16 + 16 | 32 + 16
static const int MAX_CHUNKS_LOD[LODMAX] =
{
    32, 64, 100, 600
};

extern int LOD_DENSITY_SIZES[LODMAX];
extern int LOD_VERTEX_SIZES[LODMAX];


#define CHUNK_WORLD_SIZE 20
typedef struct ChunkData
{

    int id0;
    int id1;
    int lod;
    bool within_frustum;
    uint32_t uID;
    ivec3 min;
    ivec3 max;
    ivec3 center;
    int replacement_index;
    int replacement_lod;
    bool is_child;
    bool free;
    
} ChunkData;


typedef struct MarchingCubesData
{

    int EDGE_TO_VERTICES[12][2];
    int EDGE_TABLE[256];
    int TRIANGLE_TABLE[256][16];

} MarchingCubesData;

typedef struct Vertex
{
    vec3 pos;
    float pad0;
    vec3 normal;
    float pad1;
    vec3 color;
    float pad2;
    
} Vertex;

typedef struct AtmosphereVertex
{
    vec3 pos;
    vec3 normal;

} AtmosphereVertex;

typedef struct WaterVertex
{
    vec3 pos;
} WaterVertex;


typedef struct WarpLayer
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
    
} WarpLayer;

typedef struct PerlinLayer
{

    float freq;
    float amp;
    float pad0;
    float pad1;

} PerlinLayer;

typedef struct FBMLayer
{
    
    float freq;
    float amp;
    float pad0;
    float pad1;

} FBMLayer;

#define MAX_LAYERS 24
typedef struct PlanetData
{

    int warp_count;
    int perlin_count;
    int fbm_count;
    int pad0;
    vec4 sun_dir;
    vec4 sun_color;
    WarpLayer warp_layers[MAX_LAYERS];
    PerlinLayer perlin_layers[MAX_LAYERS];
    FBMLayer fbm_layers[MAX_LAYERS];
    
} PlanetData;

#define WATER_FFT_TEXTURE_SIZE 128
#define WATER_FFT_L 1000

typedef struct WaterData
{
    vec4 water_data[WATER_FFT_TEXTURE_SIZE * WATER_FFT_TEXTURE_SIZE];
    
} WaterData;


typedef struct Uniform
{
    GLuint location;
    char name[256];
} Uniform;

typedef struct ComputeShaderProgram
{
    char path[256];
    GLuint program;

    Uniform uniforms[32];
    
    
} ComputeShaderProgram;

typedef struct ShaderProgram
{

    GLuint program;
    char vert_path[256];
    char frag_path[256];

    Uniform uniforms[32];
    
} ShaderProgram;

// 1 chunks max lod
// 27 chunks half lod
// 90 chunks quarter

#define MAX_LOD_MAX 1
#define HALF_LOD_MAX 27
#define QUARTER_LOD_MAX 90
#define HALF_DENSITY_RESOLUTION 32
#define QUARTER_DENSITY_RESOLUTION 16


#define WATER_RES 1024
// #define WATER_RES 512

typedef struct DensityData
{
    int chunk_density_offset;
    int chunk_vertex_offset;
    float density[(MAX_DENSITY_RESOLUTION+1) * (MAX_DENSITY_RESOLUTION+1) * (MAX_DENSITY_RESOLUTION+1)];
    Vertex vertices[MAX_DENSITY_RESOLUTION * MAX_DENSITY_RESOLUTION * MAX_DENSITY_RESOLUTION * 8];
} DensityData;


#define VERTEX_COUNT_MAX (LODMAX * MAX_CHUNKS * 5)
typedef struct VertexInfo
{
    int vcnt;
    uint32_t uid;
    uint32_t pad0;
    uint32_t pad1;

} VertexInfo;
typedef struct GPUChunk
{
    float lod_dist[3];
    uint32_t emitted_chunks;
    VertexInfo vertex_counts[VERTEX_COUNT_MAX];
}GPUChunk;

float noise(vec3 p);

#define MESH_RES 64

#define ATMOSPHERE_RES 64

#define CHUNK_RADIUS 6
#define CHUNK_DIAMETER (CHUNK_RADIUS * 2 + 1)
#define CHUNK_LIMIT (CHUNK_DIAMETER * CHUNK_DIAMETER * CHUNK_DIAMETER)
#define PERLIN_TEXTURE_SIZE 32
#define WORLEY_TEXTURE_SIZE 32
#define FBM_TEXTURE_SIZE 32


extern AtmosphereVertex * ATMOSPHERE_VERTS;
extern uint32_t * ATMOSPHERE_INDICES;
extern int ATMOSPHERE_VCNT;
extern int ATMOSPHERE_ICNT;

typedef struct {
    uint32_t count;          // number of vertices
    uint32_t instanceCount;  // usually 1
    uint32_t first;          
    uint32_t baseInstance;   
} DrawArraysIndirectCommand;


extern Globals GLOBALS;


extern GLsync fence;



extern Camera cam;
extern Camera space_cam;
extern Camera LIGHT_CAM;
extern Camera * current_cam;
    
extern GLuint64 density_time;
extern GLuint64 mc_time;
extern GLuint d_q;
extern GLuint mc_q;
extern bool within_planet_atmosphere;
float noise3d(vec3 p);
void render_sky_cubemap();
#endif 
