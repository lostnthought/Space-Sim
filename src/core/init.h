#ifndef INIT_H
#define INIT_H
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "core/types.h"

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern int BLOOM_WIDTH;
extern int BLOOM_HEIGHT;
extern int BLOOM2_WIDTH;
extern int BLOOM2_HEIGHT;

extern int CURRENT_LOD_BOUND;

extern GLuint ROCK_NORMAL_TEXTURE;
extern GLuint ROCK_DIFFUSE_TEXTURE;
extern GLuint ROCK_SPECULAR_TEXTURE;
extern GLuint ROCK_AO_TEXTURE;

extern GLuint CUBEMAP_TEXTURE;
extern GLuint SKY_CUBEMAP;

extern GLuint INDIRECT_BUFFER[LODMAX][MAX_CHUNKS];
extern GLuint DENSITY_SSBO[LODMAX];
extern GLuint DENSITY_VERTEX_SSBO[LODMAX];
extern GLuint MAIN_TEXTURE;
extern GLuint WATER_TEXTURE;
extern GLuint SKY_TEXTURE;
extern GLuint DEBUG_TEXTURE;
extern GLuint SPHERE_TEXTURE;

extern GLuint WATER_FFT_PING_TEXTURE;
extern GLuint WATER_FFT_PONG_TEXTURE;
extern GLuint WATER_FFT_PING_TEXTURE_2;
extern GLuint WATER_FFT_PONG_TEXTURE_2;

extern GLuint ID_TEXTURE;
extern GLuint DEPTH_TEXTURE;
extern GLuint EXP_LOOKUP_TEXTURE;
extern GLuint COMPOSITE_TEXTURE;
extern GLuint COMPOSITE_DEPTH_TEXTURE;
extern GLuint POSTPROCESS_TEXTURE;
extern GLuint HIGHLIGHT_TEXTURE;
extern GLuint BLOOM_TEXTURE;
extern GLuint BLUR_TEXTURE;
extern GLuint BLOOM2_TEXTURE;
extern GLuint BLUR2_TEXTURE;
extern GLuint HISTORY_TEXTURE[2];
extern GLuint PERLIN_TEXTURE;
extern GLuint WORLEY_TEXTURE;
extern GLuint FBM_TEXTURE;
extern GLuint SHADOW_MAP_TEXTURE;
extern GLuint MAIN_FBO;
extern GLuint SKY_FBO;
extern GLuint POSTPROCESS_FBO;
extern GLuint COMPOSITE_FBO;
extern GLuint BLOOM_FBO;
extern GLuint BLUR_FBO;
extern GLuint BLOOM2_FBO;
extern GLuint BLUR2_FBO;
extern GLuint SHADOW_MAP_FBO;
extern GLuint COMPOSITE_FBO;
extern GLuint HISTORY_FBO[2];
extern GLuint MAIN_FRAG;

extern ShaderProgram SHADOW_MAP_SHADER;
extern ShaderProgram SHADOW_WATER_SHADER;
extern ShaderProgram MAIN_SHADER;
extern ShaderProgram MESH_SHADER;
extern ShaderProgram SKY_SHADER;
extern ShaderProgram WATER_SHADER;
extern ShaderProgram FULLSCREEN_SHADER;
extern ShaderProgram POSTPROCESS_SHADER;
extern ShaderProgram HISTORY_SHADER;
extern ShaderProgram BLOOM_SHADER;
extern ShaderProgram BLUR_SHADER;
extern ShaderProgram COMPOSITE_SHADER;
extern ShaderProgram ATMOSPHERE_SHADER;
extern ShaderProgram CLOUDS_SHADER;
extern ShaderProgram ATMOSPHERE_FROM_SPACE_SHADER;


extern GLuint MAIN_COMPUTE_SHADER;
extern ComputeShaderProgram DENSITY_COMPUTE_SHADER;
extern ComputeShaderProgram MARCHING_CUBES_COMPUTE_SHADER;
extern ComputeShaderProgram INDIRECT_COMPUTE_SHADER;
extern GLuint FORCE1_COMPUTE_SHADER;
extern GLuint FORCE2_COMPUTE_SHADER;
extern GLuint ZERO_COMPUTE_SHADER;
extern GLuint CHUNK_ZERO_COMPUTE_SHADER;
extern GLuint BOUNDS_COMPUTE_SHADER;
extern GLuint MORTON_COMPUTE_SHADER;
extern GLuint RADIX_COMPUTE_SHADER;
extern GLuint RADIX_GLOBAL_COMPUTE_SHADER;
extern GLuint RADIX_FINAL_COMPUTE_SHADER;
extern GLuint TREE_COMPUTE_SHADER;
extern GLuint TREE_COUNT_COMPUTE_SHADER;
extern GLuint COM_COMPUTE_SHADER;
extern GLuint NEXT_COMPUTE_SHADER;
extern GLuint DEPTH_COMPUTE_SHADER;
extern GLuint SPAWN_COMPUTE_SHADER;
extern GLuint POS_COMPUTE_SHADER;
extern GLuint COLLISION_COMPUTE_SHADER;
extern GLuint WATER_COMPUTE_SHADER;
extern GLuint BIT_REVERSAL_COMPUTE_SHADER;
extern GLuint WATER_FFT_COMPUTE_SHADER;
extern GLuint * CURRENT_WATER_FFT_TEXTURE;
extern GLuint MAIN_VBO;
extern GLuint MAIN_VAO;
extern GLuint SKY_VBO;
extern GLuint WATER_VBO;
extern GLuint SKY_VAO;
extern GLuint WATER_VAO;
extern GLuint WATER_EBO;
extern uint32_t water_quad_count;
extern uint32_t water_index_count;
extern GLuint MESH_VBO;
extern GLuint MESH_EBO;
extern GLuint MESH_VAO;
extern GLuint ATMOSPHERE_VBO;
extern GLuint ATMOSPHERE_EBO;
extern GLuint ATMOSPHERE_VAO;

extern GLuint FS_VAO;
extern GLuint CAM_UBO;
extern GLuint SPACE_CAM_UBO;
extern GLuint RENDER_UBO;
extern GLuint PARTICLE_SSBO;
extern GLuint WATER_SSBO;
extern GLuint MORTON_SSBO;
extern GLuint MORTON_PING_SSBO;
extern GLuint BOUNDS_SSBO;
extern GLuint NODE_SSBO;
extern GLuint NODE_COUNTER_SSBO;
extern GLuint RADIX_SSBO;
extern GLuint MARCHING_CUBES_SSBO;
extern GLuint POSITION_SSBO;
extern GLuint CHUNK_DATA_SSBO[3];
extern int * CHUNK_DATA_PTR[3];
extern GLuint PLANET_DATA_SSBO;
extern int CHUNK_DATA_FLAG;

extern ChunkData CHUNK_DATA[LODMAX][MAX_CHUNKS];

extern PlanetData PLANET_DATA;

extern bool NUKLEAR_HOVERED;
extern float * perlin_tex;
extern float * fbm_tex;

extern struct nk_context * NK_CTX;


/**
 * @ingroup atmosphere_group
 * @{
 */
extern float ESun;
extern vec3 wavelength;
extern vec3 cam_pos_world;
extern vec3 sun_dir;
extern vec3 sun_color;
extern float G;
extern float fKr;
extern float fKm;
 /** @} */

extern bool dirty_cubemap;
extern ParticleData PD;
extern float MASS;

extern ChunkScore * jobs;
extern ChunkData * job_data;
extern ChunkScore * frees;

extern vec3 planet_origin;

void init_random();


void update_water_fft(float t);


typedef struct Shader
{
    GLuint program;
    GLuint locations[10];
    int uniforms;
    
} Shader;


bool init(SDL_Window **window, SDL_GLContext *context);

#endif
