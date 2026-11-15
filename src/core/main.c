#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include "core/camera.h"
#include "core/init.h"
#include "utilities/mathf.h"
#include "core/menu.h"
#include "mesh/mesh.h"
#include "core/shaders.h"
#include "core/types.h"
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <immintrin.h>
#include <stdalign.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 960
#define STB_IMAGE_IMPLEMENTATION

#include "stb/stb_image.h"

#include "nk_config.h"
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

Globals GLOBALS = {0};

int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 960;

int BLOOM_WIDTH = 1280 / 2;
int BLOOM_HEIGHT = 960 / 2;
int BLOOM2_WIDTH = 1280 / 4;
int BLOOM2_HEIGHT = 960 / 4;

// Placeholder Textures
// per planet type
GLuint ROCK_NORMAL_TEXTURE;
GLuint ROCK_DIFFUSE_TEXTURE;
GLuint ROCK_SPECULAR_TEXTURE;
GLuint ROCK_AO_TEXTURE;

GLuint CUBEMAP_TEXTURE;

/**
 * @ingroup mesh_group
 * @{
 */
// Chunk Settings
int CURRENT_LOD_BOUND = -1;
int LOD_DENSITY_SIZES[LODMAX];
int LOD_VERTEX_SIZES[LODMAX];
// Chunk Data
ChunkData CHUNK_DATA[LODMAX][MAX_CHUNKS];
int *CHUNK_DATA_PTR[3];
int CHUNK_DATA_FLAG = 0;

 /** @} */

// Textures
GLuint MAIN_TEXTURE;
GLuint SKY_CUBEMAP;
GLuint COMPOSITE_TEXTURE;
GLuint COMPOSITE_DEPTH_TEXTURE;
GLuint EXP_LOOKUP_TEXTURE;
GLuint SKY_TEXTURE;
GLuint DEBUG_TEXTURE;
GLuint ID_TEXTURE;
GLuint SPHERE_TEXTURE;
GLuint DEPTH_TEXTURE;
GLuint BLUR_TEXTURE;
GLuint WATER_TEXTURE;
GLuint BLUR2_TEXTURE;
GLuint BLOOM_TEXTURE;
GLuint BLOOM2_TEXTURE;
GLuint HISTORY_TEXTURE[2];
GLuint POSTPROCESS_TEXTURE;
GLuint WORLEY_TEXTURE;
GLuint PERLIN_TEXTURE;
GLuint FBM_TEXTURE;
GLuint SHADOW_MAP_TEXTURE;

/**
 * @ingroup water_group
 * @{
 */
GLuint WATER_FFT_PING_TEXTURE;
GLuint WATER_FFT_PONG_TEXTURE;
GLuint WATER_FFT_PING_TEXTURE_2;
GLuint WATER_FFT_PONG_TEXTURE_2;
 /** @} */

// FBOs
GLuint MAIN_FBO;
GLuint COMPOSITE_FBO;
GLuint SKY_FBO;
GLuint POSTPROCESS_FBO;
GLuint HISTORY_FBO[2];
GLuint BLOOM_FBO;
GLuint BLOOM2_FBO;
GLuint BLUR_FBO;
GLuint BLUR2_FBO;
GLuint SHADOW_MAP_FBO;

// Shaders
ShaderProgram SHADOW_MAP_SHADER;
ShaderProgram SHADOW_WATER_SHADER;
ShaderProgram MAIN_SHADER;
ShaderProgram MESH_SHADER;
ShaderProgram SKY_SHADER;
ShaderProgram WATER_SHADER;
ShaderProgram FULLSCREEN_SHADER;
ShaderProgram POSTPROCESS_SHADER;
ShaderProgram HISTORY_SHADER;
ShaderProgram COMPOSITE_SHADER;
ShaderProgram BLOOM_SHADER;
ShaderProgram BLUR_SHADER;
ShaderProgram COMPOSITE_SHADER;
ShaderProgram ATMOSPHERE_SHADER;
ShaderProgram ATMOSPHERE_FROM_SPACE_SHADER;
ShaderProgram CLOUDS_SHADER;

// Compute Shaders
GLuint MAIN_COMPUTE_SHADER;
ComputeShaderProgram DENSITY_COMPUTE_SHADER;
ComputeShaderProgram MARCHING_CUBES_COMPUTE_SHADER;
ComputeShaderProgram INDIRECT_COMPUTE_SHADER;
GLuint CHUNK_ZERO_COMPUTE_SHADER;
GLuint FORCE1_COMPUTE_SHADER;
GLuint FORCE2_COMPUTE_SHADER;
GLuint ZERO_COMPUTE_SHADER;
GLuint BOUNDS_COMPUTE_SHADER;
GLuint MORTON_COMPUTE_SHADER;
GLuint RADIX_COMPUTE_SHADER;
GLuint RADIX_GLOBAL_COMPUTE_SHADER;
GLuint RADIX_FINAL_COMPUTE_SHADER;
GLuint TREE_COMPUTE_SHADER;
GLuint TREE_COUNT_COMPUTE_SHADER;
GLuint COM_COMPUTE_SHADER;
GLuint DEPTH_COMPUTE_SHADER;
GLuint NEXT_COMPUTE_SHADER;
GLuint SPAWN_COMPUTE_SHADER;
GLuint POS_COMPUTE_SHADER;
GLuint COLLISION_COMPUTE_SHADER;
GLuint WATER_COMPUTE_SHADER;
GLuint WATER_FFT_COMPUTE_SHADER;
GLuint BIT_REVERSAL_COMPUTE_SHADER;

// Buffers
GLuint MAIN_VBO;
GLuint SKY_VBO;
GLuint WATER_VBO;
GLuint WATER_EBO;
uint32_t water_quad_count;
uint32_t water_index_count;
GLuint MAIN_VAO;
GLuint SKY_VAO;
GLuint WATER_VAO;
GLuint MESH_VBO;
GLuint MESH_EBO;
GLuint MESH_VAO;
GLuint FS_VAO;
GLuint INDIRECT_BUFFER[LODMAX][MAX_CHUNKS];


/**
 * @ingroup atmosphere_group
 * @{
 */
GLuint ATMOSPHERE_VBO;
GLuint ATMOSPHERE_EBO;
GLuint ATMOSPHERE_VAO;
AtmosphereVertex *ATMOSPHERE_VERTS;
uint32_t *ATMOSPHERE_INDICES;
int ATMOSPHERE_VCNT;
int ATMOSPHERE_ICNT;
 /** @} */



PlanetData PLANET_DATA;

// UBOs
GLuint CAM_UBO;
GLuint SPACE_CAM_UBO;
GLuint RENDER_UBO;

// SSBOs
GLuint PARTICLE_SSBO;
GLuint MORTON_SSBO;
GLuint MORTON_PING_SSBO;
GLuint BOUNDS_SSBO;
GLuint NODE_COUNTER_SSBO;
GLuint NODE_SSBO;
GLuint RADIX_SSBO;
GLuint DENSITY_SSBO[LODMAX];
GLuint DENSITY_VERTEX_SSBO[LODMAX];
GLuint MARCHING_CUBES_SSBO;
GLuint POSITION_SSBO;
GLuint PLANET_DATA_SSBO;
GLuint WATER_SSBO;
GLuint CHUNK_DATA_SSBO[3];


// Flag for if the application is running
int running = 0;

// Cameras (Swaps between because space coords != planet coords)
Camera cam = {0};
Camera space_cam = {0};
Camera LIGHT_CAM;
Camera *current_cam = &cam;
float move_speed = 0.1f;

// Delta time (frame)
float dt;
// Delta time (particles)
float DT = 0.5f;

// Rayleigh / Mie starting values
float ESun = 20.0f;
vec3 wavelength = {0.650f, 0.570f, 0.475f};
vec3 cam_pos_world = {0.01f, 0.01f, 1.001f};
vec3 sun_dir = {0.1f, 0.2f, -0.8f};
vec3 sun_color = {1.0, 0.7, 0.5};
float G = -0.88f;
float fKr = 0.0025f;
float fKm = 0.001f;

// Particle data structures (SSBO)
ParticleData PD;
NodeData ND;

GLsync fence;

ParticleVertex verts[BODY_COUNT];
Particle bodies[BODY_COUNT] = {0};
Node nodes[MAX_NODES] = {0};


int frame_available = 0;
bool display_menu = true;
bool halt_chunks = false;

// info based on current planet for mesh calculations
bool is_on_planet = true;
vec3 planet_origin = (vec3){0, 0, 0};
vec4 mat = (vec4){0, 0, 0, 0};
float planet_radius = 1000;
GLuint main_queries[QUERY_FRAMES];
GLuint bounds_queries[QUERY_FRAMES];
GLuint morton_queries[QUERY_FRAMES];
GLuint radix_queries[QUERY_FRAMES];
GLuint treecount_queries[QUERY_FRAMES];
GLuint tree_queries[QUERY_FRAMES];
GLuint com_queries[QUERY_FRAMES];
GLuint atmosphere_queries[QUERY_FRAMES];
GLuint clouds_queries[QUERY_FRAMES];
GLuint water_gen_queries[QUERY_FRAMES];
GLuint water_queries[QUERY_FRAMES];
GLuint mesh_generation_queries[QUERY_FRAMES];
GLuint particle_queries[QUERY_FRAMES];
GLuint mesh_queries[QUERY_FRAMES];
GLuint draw_queries[QUERY_FRAMES];
GLuint postprocess_queries[QUERY_FRAMES];

int current_query_frame = 1;





// initializes particle values to a solid distribution


int frame = 0;
uint32_t *level_flag_ptr = NULL;


#define NUM_WORKGROUPS 128


GLuint64 retrieve_gl_query(GLuint query)
{
    GLint available = 0;
    glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
    GLuint64 e = 0;
    if (available)
    {

        glGetQueryObjectui64v(query, GL_QUERY_RESULT, &e);
    }
    return e;
}

// Particle simulation iteration

/**
 * @brief A single simulation step for the particle sim.
 * @details
 * \begin{align}
 * & \text{DEF: } \\\\
 * & \quad P = \text{%Particles} \\\\
 * & \quad N = \text{%Nodes} \\\\
 * & \quad N_{lr} = \text{%Node Children} \\\\
 * & \quad N_{m} = \text{%Node Mass} \\\\
 * & \quad N_{\text{com}} = \text{%Node Center of Mass} \\\\
 * & \quad N_{\text{n}} = \text{%Node Next} \\\\
 * M(D) &= \underset{x \in D}{\max} x_{\text{xyz}} \\\\
 * m(D) &= \underset{x \in D}{\min} x_{\text{xyz}} \\\\
 * \mathit{max} &= M(P) \\\\
 * \mathit{min} &= m(P) \\\\
 * \Delta &= \mathit{max} - \mathit{min} \\\\
 * \mathcal B &= \max(\max(\Delta_x, \Delta_y), \Delta_z) \\\\
 * f(p) &= \begin{cases}
 * c = \frac{p_{xyz} - \mathit{min}}{\mathcal B}\\\\
 * \text{Morton}(c)
 * \end{cases} \\\\
 * M &= \lbrack f(p) | p \in P \rbrack \\\\
 * M\prime &= M \to M_1, M_2, ... M_n \\\\
 * o(x, y) &= \begin{cases}
 * n = 31 \\\\
 * \text{while } x[n] = y[n] \text{ and } n \ge 0: \\\\
 * \quad n = n - 1 \\\\
 * n 
 * \end{cases} \\\\
 * & \text{Where each index into x and y is a bit.} \\\\
 * r(x, y) &= \begin{cases}
 * -1, &\text{if } y \lt 0 \text{ or } y \ge P_{\text{count}} - 1 \\\\
 * o(M\prime_x, M\prime_y), &\text{otherwise}
 * \end{cases} \\\\
 * S(i) &= \begin{cases}
 * l_p = \begin{cases}
 * o(M\prime_i, M\prime_{i-1}), & \text{if }i \gt 0 \\\\
 * -1, & \text{otherwise}
 * \end{cases} \\\\
 * r_p = \begin{cases}
 * o(M\prime_i, M\prime_{i+1}), & \text{if }i \lt P_{\text{count}}-1 \\\\
 * -1, & \text{otherwise}
 * \end{cases} \\\\
 * \vec d = \begin{cases}
 * 1, & \text{if } r_p > l_p \\\\
 * -1, & \text{otherwise} \\\\
 * \end{cases} \\\\
 * \Delta_{\text{min}} = r(i, i - \vec{d}) \\\\
 * h = 2 \\\\
 * k = i + h \vec{d} \\\\
 * \text{while } k \ge 0 \text{ and } g \lt P_{\text{count}}: \\\\
 * \quad \text{break}, & \text{if } o(M\prime_i, M\prime_k) \le \Delta_{\text{min}} \\\\
 * \quad h = h * 2 \\\\
 * \quad k = i + h \vec{d} \\\\
 * l = 0 \\\\
 * t = \frac{h}{2} \\\\
 * k = i + (l + t) \vec{d} \\\\
 * \text{while } t \ge 1 \text{ and } k \ge 0 \text{ and } k \lt P_{\text{count}}: \\\\
 * \quad l = l + t, & \text{if } o(M\prime_i, M\prime_k) \gt \Delta_{\text{min}} \\\\
 * j = i + l \vec{d} \\\\
 * v = \min(i, j) \\\\
 * z = \max(i, j) \\\\
 * s_\perp = v \\\\
 * s = z - v \\\\
 * \delta = r(v, z) \\\\
 * \text{while } s \gt 1: \\\\
 * \quad s = \lfloor (s + 1) \ 2 \rfloor \\\\
 * \quad s_{\perp 1} = s_\perp + s \\\\
 * \quad \left. \begin{aligned}
 * e = r(v, s_{\perp 1}) \\\\
 * s_\perp = s{\perp 1}a
 * \end{aligned} \right\rbrace, & \text{if } e \gt \delta\\\\
 * n_i = P_{\text{count}} + i \\\\
 * \begin{cases}
 * N[n_i]_l = v, & \text{if } s_\perp = v \\\\
 * N[n_i]_l = P_{\text{count}} + s, & \text{otherwise}
 * \end{cases} \\\\
 * \begin{cases}
 * N[n_i]_r = s_\perp + 1, & \text{if } s_\perp + 1 = z \\\\
 * N[n_i]_r = s_\perp + 1 + P_{\text{count}}, & \text{otherwise}
 * \end{cases} \\\\
 * \end{cases} \\\\
 * & (S(i) | i \in [0, P_{\text{count}})) \\\\
 * Q(i) &= \begin{cases}
 * Q(N[i]_l) \\\\
 * Q(N[i]_r) \\\\
 * l = N[N[i]_l] \\\\
 * r = N[N[i]_r] \\\\
 * N[i]_{\text{com}} = \frac{l_{\text{com}} l_m + r_{\text{com}} r_m}{l_m + r_m} \\\\
 * N[i]_{\text{min}} = \min(l_{\text{min}}, r_{\text{min}}) \\\\
 * N[i]_{\text{max}} = \min(l_{\text{max}}, r_{\text{max}}) \\\\
 * d = N[i]_{\text{max}} - N[i]_{\text{min}} \\\\
 * N[i]_r = \max(\max(d_x, d_y), d_z)^2 \\\\
 * \end{cases} \\\\
 * & Q(P_{\text{count}}) \\\\
 * & \text{Where } P_{\text{count}} \text{ is the root node.} \\\\
 * & \text{Each node stores the node to continue with if its branch is skipped.} \\\\
 * U(i) &= \begin{cases}
 * a = 0 \\\\
 * n = 0 \\\\
 * \text{While }n \ge 0: \\\\
 * \quad \left. \begin{aligned}
 * n = N[n]_n \\\\
 * \text{continue}
 * \end{aligned} \right\},  \text{if } N[n]_m = 0 \\\\
 * r = (N[n]_{\text{com}} - P[i]_{\text{pos}}) \\\\
 * d = r^2 + \beta \\\\
 * \quad \begin{cases} \left. \begin{aligned}
 * a = a + r \cdot (N[n]_m \cdot (\frac{1}{\sqrt{d}})^3) \\\\
 * n = N[n]_n \\\\
 * \text{continue}
 * \end{aligned} \right\},  \text{if n is a leaf}\\\\
 * \begin{cases}
 * \left. \begin{aligned}
 * a = a + r \cdot (N[n]_m \cdot (\frac{1}{\sqrt{d}})^3) \\\\
 * n = N[n]_n \\\\
 * \text{continue}
 * \end{aligned} \right\},  \text{if } \frac{N[n]_r}{d} \lt \theta^2 \\\\
 * n = N[n]_l, \text{otherwise}
 * \end{cases} \\\\
 * \end{cases} \\\\
 * P[i]_{\text{vel}} = P[i]_{\text{vel}} + a \Delta t \\\\
 * P[i]_{\text{pos}} = P[i]_{\text{pos}} + P[i]_{\text{vel}} \Delta t \\\\
 * \end{cases} \\\\
 * & (U(i) | i \in [0, P_{\text{count}}))
 * \end{align}
 * @ingroup particle_group
 */
void
    sim_step
    ()

{

    glBeginQuery(GL_TIME_ELAPSED, bounds_queries[current_query_frame]);

    // Zeros out certain values necessary for each iteration
    glUseProgram(ZERO_COMPUTE_SHADER);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Computes the bounds of all particles
    glUseProgram(BOUNDS_COMPUTE_SHADER);
    glDispatchCompute(
        (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
        1,
        1
    );
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, morton_queries[current_query_frame]);
    // Assigns mortons to each particle
    glUseProgram(MORTON_COMPUTE_SHADER);
    glDispatchCompute(
        (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
        1,
        1
    );
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, radix_queries[current_query_frame]);
    // Performs a radix sort over all mortons
    int wg_count = (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS;
    for (int i = 0; i < 8; i++) {

        glUseProgram(RADIX_COMPUTE_SHADER);
        glUniform1i(
            glGetUniformLocation(
                RADIX_COMPUTE_SHADER,
                "iteration"
            ),
            i
        );
        glDispatchCompute(
            (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
            1,
            1
        );
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(RADIX_GLOBAL_COMPUTE_SHADER);
        glUniform1i(
            glGetUniformLocation(
                RADIX_GLOBAL_COMPUTE_SHADER,
                "workgroup_count"
            ),
            wg_count
        );

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(RADIX_FINAL_COMPUTE_SHADER);
        glUniform1i(
            glGetUniformLocation(
                RADIX_FINAL_COMPUTE_SHADER,
                "iteration"
            ),
            i
        );
        glDispatchCompute(
            (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
            1,
            1
        );
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    glEndQuery(GL_TIME_ELAPSED);

    // Initializes nodes and particles to values necessary for build
    glBeginQuery(GL_TIME_ELAPSED, treecount_queries[current_query_frame]);
    glUseProgram(TREE_COUNT_COMPUTE_SHADER);
    glDispatchCompute(
        (MAX_NODES + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
        1,
        1
    );
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glEndQuery(GL_TIME_ELAPSED);

    // Builds the LBVH
    glBeginQuery(GL_TIME_ELAPSED, tree_queries[current_query_frame]);
    glUseProgram(TREE_COMPUTE_SHADER);
    glDispatchCompute(
        (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
        1,
        1
    );
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glEndQuery(GL_TIME_ELAPSED);

    // Propagates mass from leaves up to the root at BODY_COUNT - 1
    glBeginQuery(GL_TIME_ELAPSED, com_queries[current_query_frame]);
    glUseProgram(COM_COMPUTE_SHADER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, NODE_SSBO);

    uint32_t depth_offsets[MAX_DEPTH + 1];
    memset(depth_offsets, 0, sizeof(depth_offsets));
    depth_offsets[0] = BODY_COUNT;


    for (int i = 0; i < MAX_DEPTH; i++) {
        glUniform1i(
            glGetUniformLocation(
                COM_COMPUTE_SHADER,
                "depth"
            ),
            i
        );
        glDispatchCompute(
            (MAX_NODES + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
            1,
            1
        );

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Linearizes the tree by assigning a next index (the parent's right node) so we only have to traverse through left nodes
    glUseProgram(NEXT_COMPUTE_SHADER);
    for (int i = MAX_DEPTH - 1; i >= 0; i--) {

        glUniform1i(glGetUniformLocation(NEXT_COMPUTE_SHADER, "depth"), i);
        glDispatchCompute(
            (MAX_NODES + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
            1,
            1
        );

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    
    glEndQuery(GL_TIME_ELAPSED);

    // Calculates velocity and resolves collisions
    glBeginQuery(GL_TIME_ELAPSED, main_queries[current_query_frame]);
    glUseProgram(MAIN_COMPUTE_SHADER);
    glDispatchCompute(
        (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
        1,
        1
    );
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Spawns particles based on accretion

    // Integrates positions
    glUseProgram(POS_COMPUTE_SHADER);
    glUniform1f(
        glGetUniformLocation(
            POS_COMPUTE_SHADER,
            "DT"
        ),
        DT
    );
    glDispatchCompute(
        (BODY_COUNT + NUM_WORKGROUPS - 1) / NUM_WORKGROUPS,
        1,
        1
    );
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glEndQuery(GL_TIME_ELAPSED);

    

}

// Process keyboard / mouse input
/**
 * @brief Processes SDL events.
 */
void
    process_input
    ( SDL_Window **window )

{

    vec3 move_dir = {0};
    
    Uint32 buttons = SDL_GetMouseState(NULL, NULL);
    bool lmouse_down = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);

    // Selects a planet
    if (lmouse_down && !NUKLEAR_HOVERED) {

        if (GLOBALS.hovered_body != -1) {

            GLOBALS.selected_body = GLOBALS.hovered_body;

            Particle test;
            glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 60);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, PARTICLE_SSBO);

            Particle *ptr = glMapBufferRange(
                GL_SHADER_STORAGE_BUFFER,
                offsetof(ParticleData, p) + sizeof(Particle) * GLOBALS.selected_body,
                sizeof(Particle), GL_MAP_READ_BIT
            );

            memcpy(&test, ptr, sizeof(Particle));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            planet_origin = (vec3){test.p_m.x, test.p_m.y, test.p_m.z};
            mat = test.mat;

        }

    }


    nk_input_begin(NK_CTX);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        
        if (!SDL_GetRelativeMouseMode()) {
            nk_sdl_handle_event(&event);
        }

        // Exit
        if (event.type == SDL_QUIT) running = 0;

        // Look
        if (event.type == SDL_MOUSEMOTION && SDL_GetRelativeMouseMode()) {

            int dx = event.motion.xrel;
            int dy = event.motion.yrel;

            float sensitivity = 0.0012f;
            current_cam->yaw -= dx * sensitivity;

            current_cam->pitch -= dy * sensitivity;

            float max_pitch = (M_PI / 2.0f);
            float min_pitch = -(M_PI / 2.0f) + 0.01f;
            current_cam->pitch =
                SDL_clamp(current_cam->pitch, min_pitch, max_pitch);
            
        }

    }

    nk_input_end(NK_CTX);

    const Uint8 *keystates = SDL_GetKeyboardState(NULL);

    // Exit
    if (keystates[SDL_SCANCODE_ESCAPE]) running = 0;
    
    // WASD
    if (keystates[SDL_SCANCODE_W]) move_dir.x += 1.0f;
    if (keystates[SDL_SCANCODE_S]) move_dir.x -= 1.0f;
    if (keystates[SDL_SCANCODE_A]) move_dir.y += 1.0f;
    if (keystates[SDL_SCANCODE_D]) move_dir.y -= 1.0f;

    // Unlock mouse
    if (keystates[SDL_SCANCODE_P]) {

        SDL_SetWindowGrab(*window, SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_FALSE);

    }

    // Lock mouse
    if (keystates[SDL_SCANCODE_O]) {

        SDL_SetWindowGrab(*window, SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_TRUE);

    }

    // Old functionality for visualizing particles TODO
    // if (keystates[SDL_SCANCODE_E]) look_at_selected_body();
    // if (keystates[SDL_SCANCODE_TAB]) GLOBALS.lock_on = !GLOBALS.lock_on;

    // Jump
    if (keystates[SDL_SCANCODE_SPACE] && current_cam->on_ground) {

        cam.cam_data.cam_vel.z += 0.1f;
        cam.on_ground = false;

    }

    // Noclip
    if (keystates[SDL_SCANCODE_T]) is_on_planet = !is_on_planet;
    
    // Pause particle sim
    if (keystates[SDL_SCANCODE_R]) {

        if (GLOBALS.paused) {
            DT = 0.05f;
            GLOBALS.paused = false;
        } else {
            DT = 0;
            GLOBALS.paused = true;
        }

    }

    // Reload shaders
    if (keystates[SDL_SCANCODE_Y]) init_shaders();

    // Halt chunks
    if (keystates[SDL_SCANCODE_U]) halt_chunks = !halt_chunks;

    // Display menu
    if (keystates[SDL_SCANCODE_Q]) display_menu = !display_menu;

    // Apply velocity and resolve collisions
    if (is_on_planet) {

        apply_planet_velocity(current_cam, move_dir, dt);
        resolve_collisions(current_cam, planet_radius);
        
    } else {
        
        apply_velocity(current_cam, move_dir, dt);

    }

}

float uTime = 0;
int cur_history = 0;
int chunk_count = 0;

// Renders a planet from outside its atmosphere (Only used on selected planet)
// I never really got the visual artifacts worked out on this, not sure what I'm missing
/**
 * @brief Renders the atmosphere from outside of the atmosphere.
 * @see https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering
 * @ingroup atmosphere_group
 */
void
    render_sky_from_outside_atmosphere
    ()

{

    glUseProgram(ATMOSPHERE_FROM_SPACE_SHADER.program);
    glBindVertexArray(ATMOSPHERE_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, ATMOSPHERE_VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ATMOSPHERE_EBO);

    // Moves cam into local planet space and scales by a realistic margin
    float inner_radius = 1;
    float outer_radius_factor = 1000;
    cam_pos_world.z = (cam.cam_data.cam_pos.z - planet_origin.z) / outer_radius_factor;
    
    glUniform3f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[0].location,
        cam_pos_world.x, cam_pos_world.y, cam_pos_world.z
    );

    // inner radius
    // inner radius ** 2
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[1].location,
        inner_radius
    );
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[2].location,
        inner_radius * inner_radius
    );

    // outer radius
    // outer radius ** 2
    float outer_radius = 1.1;
    // float outer_radius = inner_radius + inner_radius * 0.2f;
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[3].location,
        outer_radius
    );
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[4].location,
        outer_radius * outer_radius
    );

    vec3 inv;
    inv.x = 1.0f / powf(wavelength.x, 4.0f);
    inv.y = 1.0f / powf(wavelength.y, 4.0f);
    inv.z = 1.0f / powf(wavelength.z, 4.0f);
    glUniform3f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[5].location,
        inv.x, inv.y, inv.z
    );

    // fscale -> 1 / (outer radius - inner radius)
    float fscale = 1 / (outer_radius - inner_radius);
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[6].location,
        fscale
    );

    const float fscaledepth = 1.0f;
    // const float fscaledepth = 50;
    // fscale / fscaledepth
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[7].location,
        fscale / fscaledepth
    );

    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[8].location,
        1 / fscaledepth
    );

    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[9].location,
        fscaledepth
    );

    // g
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[10].location,
        G
    );

    // g ** 2
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[11].location,
        G * G
    );

    // fKrESun
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[12].location,
        fKr * ESun
    );

    // fKmESun
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[13].location,
        fKm * ESun
    );

    // fKr4Pi
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[14].location,
        fKr * 4 * M_PI
    );

    // fKm4Pi
    glUniform1f(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[15].location,
        fKm * 4 * M_PI
    );

    // TODO cache these positions
    glUniformMatrix4fv
    (
        glGetUniformLocation
        (
            ATMOSPHERE_FROM_SPACE_SHADER.program,
            "uFakeProj"
        ),
        1,
        0,
        (GLfloat *)&cam.cam_data.proj
    );

    glUniformMatrix4fv
    (
        ATMOSPHERE_FROM_SPACE_SHADER.uniforms[16].location,
        1,
        0,
        (GLfloat *)&cam.cam_data.view
    );
    
    glUniform3f
    (
        glGetUniformLocation
        (
            ATMOSPHERE_FROM_SPACE_SHADER.program,
            "uCamPos"
        ),
        cam.cam_data.cam_pos.x,
        cam.cam_data.cam_pos.y,
        cam.cam_data.cam_pos.z
    );

    glDrawElements
    (
        GL_TRIANGLES,
        ATMOSPHERE_ICNT,
        GL_UNSIGNED_INT,
        (const void *)0
    );

}

/**
 * @brief Renders the atmosphere and clouds from inside of the atmosphere.
 * @see https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering
 * @ingroup atmosphere_group
 */
void
    render_sky_cubemap
    ()

{

    glBeginQuery(GL_TIME_ELAPSED, atmosphere_queries[current_query_frame]);
    glBindFramebuffer(GL_FRAMEBUFFER, SKY_FBO);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_CUBE_MAP,
        SKY_CUBEMAP,
        0
    );

    glViewport(0, 0, 128, 128);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float yaws[6] = {0, M_PI, M_PI, M_PI, M_PI / 2.0f + M_PI, M_PI / 2.0f};
    float pitches[6] = {0, 0, M_PI / 2.0f, -M_PI / 2.0f, 0, 0};
    vec3 flips[6];
    flips[0] = (vec3){0, 0, -1};
    flips[1] = (vec3){0, 0, -1};
    flips[2] = (vec3){0, -1, 0};
    flips[3] = (vec3){0, -1, 0};
    flips[4] = (vec3){0, 0, -1};
    flips[5] = (vec3){0, 0, -1};

    glUseProgram(ATMOSPHERE_SHADER.program);
    glBindVertexArray(ATMOSPHERE_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, ATMOSPHERE_VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ATMOSPHERE_EBO);

    float inner_radius = 1;
    float outer_radius_factor = 1000;
    cam_pos_world.z =
        (cam.cam_data.cam_pos.z - planet_origin.z) / outer_radius_factor;

    glUniform3f(
        ATMOSPHERE_SHADER.uniforms[0].location,
        cam_pos_world.x, cam_pos_world.y, cam_pos_world.z
    );

    // inner radius
    // inner radius ** 2
    glUniform1f(ATMOSPHERE_SHADER.uniforms[1].location,
        inner_radius
    );
    glUniform1f(ATMOSPHERE_SHADER.uniforms[2].location,
        inner_radius * inner_radius
    );

    // outer radius
    // outer radius ** 2
    float outer_radius = 1.1;
    // float outer_radius = inner_radius + inner_radius * 0.2f;
    glUniform1f(ATMOSPHERE_SHADER.uniforms[3].location,
        outer_radius
    );
    glUniform1f(ATMOSPHERE_SHADER.uniforms[4].location,
        outer_radius * outer_radius
    );

    vec3 inv;
    inv.x = 1.0f / powf(wavelength.x, 4.0f);
    inv.y = 1.0f / powf(wavelength.y, 4.0f);
    inv.z = 1.0f / powf(wavelength.z, 4.0f);

    glUniform3f(ATMOSPHERE_SHADER.uniforms[5].location,
        inv.x, inv.y, inv.z
    );

    float fscale = 1 / (outer_radius - inner_radius);
    // fscale -> 1 / (outer radius - inner radius)
    glUniform1f(ATMOSPHERE_SHADER.uniforms[6].location, fscale);

    const float fscaledepth = 0.25f;
    // const float fscaledepth = 50;
    // fscale / fscaledepth
    glUniform1f(ATMOSPHERE_SHADER.uniforms[7].location,
        fscale / fscaledepth
    );
    glUniform1f(ATMOSPHERE_SHADER.uniforms[8].location,
        1 / fscaledepth
    );
    glUniform1f(ATMOSPHERE_SHADER.uniforms[9].location,
        fscaledepth
    );

    // g
    glUniform1f(ATMOSPHERE_SHADER.uniforms[10].location,
        G
    );
    // g ** 2
    glUniform1f(ATMOSPHERE_SHADER.uniforms[11].location,
        G * G
    );

    // fKrESun
    glUniform1f(ATMOSPHERE_SHADER.uniforms[12].location,
        fKr * ESun
    );
    // fKmESun
    glUniform1f(ATMOSPHERE_SHADER.uniforms[13].location,
        fKm * ESun
    );
    // fKr4Pi
    glUniform1f(ATMOSPHERE_SHADER.uniforms[14].location,
        fKr * 4 * M_PI
    );
    // fKm4Pi
    glUniform1f(ATMOSPHERE_SHADER.uniforms[15].location,
        fKm * 4 * M_PI
    );

    mat4 proj = make_projection(90, 128.0f / 128.0f, 0.01, 100.0f);

    for (int i = 0; i < 6; i++) {
        // cam.cam_data.cam_pos.z += 0.5f;
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            SKY_CUBEMAP,
            0
        );

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Camera sky_cam;
        sky_cam.yaw = yaws[i];
        sky_cam.pitch = pitches[i];

        compute_camera_basis(&sky_cam, flips[i]);

        mat4 view = make_look_at(
            &sky_cam,
            (vec3){
                cam.cam_data.cam_pos.x,
                cam.cam_data.cam_pos.y,
                cam.cam_data.cam_pos.z
            }
         );

        glUniformMatrix4fv
        (
            glGetUniformLocation
            (
                ATMOSPHERE_SHADER.program,
                "uFakeProj"
            ),
            1,
            0,
            (GLfloat *)&proj
        );
        glUniformMatrix4fv(
            ATMOSPHERE_SHADER.uniforms[16].location,
            1,
            0,
           (GLfloat *)&view
       );
        // glUniform1f(glGetUniformLocation(ATMOSPHERE_SHADER.program,
        // "uColor"), i);
        glUniform3f
        (
            glGetUniformLocation
            (
                ATMOSPHERE_SHADER.program,
                "uCamPos"
            ),
            cam.cam_data.cam_pos.x,
            cam.cam_data.cam_pos.y,
            cam.cam_data.cam_pos.z
        );

        glDrawElements(
            GL_TRIANGLES,
            ATMOSPHERE_ICNT,
            GL_UNSIGNED_INT,
            (const void *)0
        );
    }

    glEndQuery(GL_TIME_ELAPSED);

    glBeginQuery(GL_TIME_ELAPSED, clouds_queries[current_query_frame]);
    glUseProgram(CLOUDS_SHADER.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, PERLIN_TEXTURE);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, FBM_TEXTURE);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, SKY_CUBEMAP);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, WORLEY_TEXTURE);

    glUniform3f(CLOUDS_SHADER.uniforms[0].location,
        cam_pos_world.x, cam_pos_world.y, cam_pos_world.z);
    glUniform1f(CLOUDS_SHADER.uniforms[1].location,
        inner_radius);
    glUniform1i(CLOUDS_SHADER.uniforms[2].location,
        0);
    glUniform1i(CLOUDS_SHADER.uniforms[3].location,
        1);
    glUniformMatrix4fv(
                CLOUDS_SHADER.uniforms[4].location,
        1, 0, (GLfloat *)&proj
    );
    glUniform1i(glGetUniformLocation(CLOUDS_SHADER.program, "uCubemap"),
        2
    );
    glUniform1i(glGetUniformLocation(CLOUDS_SHADER.program, "worley_tex"),
        3
    );
    glUniform1f(glGetUniformLocation(CLOUDS_SHADER.program, "uOuterRadius"),
        outer_radius
    );
    glUniform1f(glGetUniformLocation(CLOUDS_SHADER.program, "uPlanetRadius"),
        planet_radius
    );

    for (int i = 0; i < 6; i++) {

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            SKY_CUBEMAP,
            0
        );

        Camera sky_cam;
        sky_cam.yaw = yaws[i];
        sky_cam.pitch = pitches[i];

        compute_camera_basis(&sky_cam, flips[i]);

        mat4 view = make_look_at(
            &sky_cam,
            (vec3){
                cam.cam_data.cam_pos.x,
                cam.cam_data.cam_pos.y,
                cam.cam_data.cam_pos.z
            }
        );
        
        glUniformMatrix4fv
        (
            CLOUDS_SHADER.uniforms[5].location,
            1,
            0,
           (GLfloat *)&view
       );

        glDrawElements(
            GL_TRIANGLES,
            ATMOSPHERE_ICNT,
            GL_UNSIGNED_INT,
            (const void *)0
        );
    }
    glEndQuery(GL_TIME_ELAPSED);
}

/**
 * @brief Renders a planet's meshed surface.
 * @ingroup mesh_group
 */
void
    render_mesh_planet
    ()

{


    for (int l = 0; l < LODMAX; l++) {

        CURRENT_LOD_BOUND = l;
        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            9,
            DENSITY_SSBO[l]
        );
        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            10,
            DENSITY_VERTEX_SSBO[l]
        );

        int lid = 0;
        for (int i = 0; i < MAX_CHUNKS_LOD[l]; i++) {

            if (CHUNK_DATA[l][i].free) continue;

            bool child = true;

            if ( CHUNK_DATA[l][i].id1 == -1 || !CHUNK_DATA[l][i].is_child )
                { child = false; }

            if (child) continue;

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, INDIRECT_BUFFER[l][i]);
            glUniform1i(MESH_SHADER.uniforms[11].location, child);
            glUniform1i(MESH_SHADER.uniforms[12].location, l);
            glUniform1i(glGetUniformLocation(MESH_SHADER.program, "cid"), lid++);
            glDrawArraysIndirect(GL_TRIANGLES, 0);
        }
    }
}


float wt = 0;
bool dirty_cubemap = false;

bool within_planet_atmosphere = true;
bool selection_mode = false;


/**
 * @brief Main render loop that updates the sky, then renders water + planet surfaces.
 */
void
    render ()

{


    wt += 0.1f;

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    cur_history ^= 1;
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    // Upload camera UBOs
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, CAM_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, CAM_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CamData), &cam.cam_data);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, SPACE_CAM_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, SPACE_CAM_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CamData), &space_cam.cam_data);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, RENDER_UBO);

    // if (dirty_cubemap)
    // {

    // Update Sky Cubemap
    if (within_planet_atmosphere && GLOBALS.sky_toggle) {

        render_sky_cubemap();

    }

    // dirty_cubemap = false;
    // }



    /*
        SKY PASS
        Render Target:
            MAIN_TEXTURE on MAIN_FBO
        Using:
            SKY_VAO
            SKY_VBO
            SKY_SHADER
            SKY_CUBEMAP
        Desc:
            Draws the cubemap to the main framebuffer.
    */
    
    glBeginQuery(GL_TIME_ELAPSED, particle_queries[current_query_frame]);
    glBindFramebuffer(GL_FRAMEBUFFER, MAIN_FBO);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    if (within_planet_atmosphere && GLOBALS.sky_toggle) {

        glUseProgram(SKY_SHADER.program);
        glBindVertexArray(SKY_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, SKY_VBO);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, SKY_CUBEMAP);

        glUniform1i(SKY_SHADER.uniforms[0].location, 0);

        glDrawArrays(GL_TRIANGLES, 0, 36);

    }
    // else
    // {
    //     // render_sky_from_outside_atmosphere();

    // }



    // If selection mode is enabled, this clears the id texture.
    if (selection_mode) {
        int n1 = -1;
        glClearTexImage(
            ID_TEXTURE,
            0,
            GL_RED_INTEGER,
            GL_INT,
            &n1
        );
    }

    /*
        PARTICLE PASS
        Render Target:
            MAIN_TEXTURE on MAIN_FBO
        Using:
            MAIN_VAO
            MAIN_VBO
            MAIN_SHADER
        Desc:
            Draws the particles to the main framebuffer.
            Uses sky cubemap to overlay transparency.
            Draws as quads with cut edges.
            Draws particle IDs to the ID_TEXTURE to be used for selections.
    */

    if (GLOBALS.particle_toggle)
    {
        
        glUseProgram(MAIN_SHADER.program);
        glBindVertexArray(MAIN_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, MAIN_VBO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, SPHERE_TEXTURE);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, PERLIN_TEXTURE);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_3D, FBM_TEXTURE);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, SKY_CUBEMAP);

        GLuint attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments);

        glUniform1i(MAIN_SHADER.uniforms[0].location, 0);
        glUniform3f(MAIN_SHADER.uniforms[1].location, cam.cam_data.cam_pos.x,
                    cam.cam_data.cam_pos.y, cam.cam_data.cam_pos.z);
        glUniform3f(MAIN_SHADER.uniforms[2].location, cam.right.x, cam.right.y,
                    cam.right.z);
        glUniform3f(MAIN_SHADER.uniforms[3].location, cam.up.x, cam.up.y, cam.up.z);
        glUniform3f(MAIN_SHADER.uniforms[4].location, cam.forward.x, cam.forward.y,
                    cam.forward.z);
        glUniform1i(glGetUniformLocation(MAIN_SHADER.program, "perlin_tex"), 1);
        glUniform1i(glGetUniformLocation(MAIN_SHADER.program, "fbm_tex"), 2);
        glUniform1i(glGetUniformLocation(MAIN_SHADER.program, "uSky"), 3);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, BODY_COUNT);
    }
    glEndQuery(GL_TIME_ELAPSED);

    // ID Texture for particle selection
    // if (selection_mode) {
    //     glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    //     glReadBuffer(GL_COLOR_ATTACHMENT1);
    //     glReadPixels(
    //         SCREEN_WIDTH / 2,
    //         SCREEN_HEIGHT / 2,
    //         1,
    //         1,
    //         GL_RED_INTEGER,
    //         GL_INT,
    //         &GLOBALS.hovered_body
    //     );
    // }

    if (within_planet_atmosphere) {


        /*
            SHADOW PASS
            Render Target:
                SHADOW_MAP_TEXTURE on SHADOW_MAP_FBO
            Using:
                MESH_VAO
                MESH_VBO
                SHADOW_SHADER
            Desc:
        */

            glBeginQuery(GL_TIME_ELAPSED, draw_queries[current_query_frame]);
            glDisable(GL_CULL_FACE);
            glBindFramebuffer(GL_FRAMEBUFFER, SHADOW_MAP_FBO);
            glClearColor(0, 0, 0, 0);
            glBindVertexArray(MESH_VAO);
            glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glClearColor(0, 0, 0, 1.0f);

            glUseProgram(SHADOW_MAP_SHADER.program);

            glUniformMatrix4fv(
                SHADOW_MAP_SHADER.uniforms[0].location,
                1,
                GL_FALSE,
                (GLfloat *)&LIGHT_CAM.cam_data.proj
            );
            glUniformMatrix4fv(
                SHADOW_MAP_SHADER.uniforms[1].location,
                1,
                GL_FALSE,
                (GLfloat *)&LIGHT_CAM.cam_data.view
            );

            glUniform1f(
                glGetUniformLocation(SHADOW_MAP_SHADER.program, "uPlanetRadius"),
                planet_radius
            );

            glBindBuffer(GL_ARRAY_BUFFER, MESH_VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MESH_EBO);

            glCullFace(GL_FRONT);
            // render_mesh_planet(true);

            glDisable(GL_CULL_FACE);
            glBindVertexArray(WATER_VAO);
            glUseProgram(SHADOW_WATER_SHADER.program);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, CUBEMAP_TEXTURE);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, WATER_FFT_PONG_TEXTURE);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, WATER_FFT_PONG_TEXTURE_2);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, SKY_TEXTURE);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_CUBE_MAP, SKY_CUBEMAP);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, DEPTH_TEXTURE);
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, MAIN_TEXTURE);
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_2D, EXP_LOOKUP_TEXTURE);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_3D, FBM_TEXTURE);

            glBindBuffer(GL_ARRAY_BUFFER, WATER_VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WATER_EBO);
            glUniform1f(SHADOW_WATER_SHADER.uniforms[0].location, uTime);
            glUniform1i(SHADOW_WATER_SHADER.uniforms[1].location, 1);
            glUniform1i(SHADOW_WATER_SHADER.uniforms[2].location, 2);
            glUniform1i(SHADOW_WATER_SHADER.uniforms[3].location, 0);
            glUniform1i(SHADOW_WATER_SHADER.uniforms[4].location, 3);
            glUniform1i(SHADOW_WATER_SHADER.uniforms[5].location, 5);
            glUniform1i(SHADOW_WATER_SHADER.uniforms[6].location, 4);
            glUniform1i(glGetUniformLocation(SHADOW_WATER_SHADER.program, "uCubemap"),
                6);
            glUniform1i(glGetUniformLocation(SHADOW_WATER_SHADER.program, "uDepth"),
                7);
            glUniform1i(glGetUniformLocation(SHADOW_WATER_SHADER.program, "uScene"),
                8);
            glUniform1i(glGetUniformLocation(SHADOW_WATER_SHADER.program, "uExp"),
                9);
            glUniform1f(
                glGetUniformLocation(SHADOW_WATER_SHADER.program, "uPlanetRadius"),
                planet_radius);
            glUniform1i(
                glGetUniformLocation(SHADOW_WATER_SHADER.program, "fbm_tex"), 10);

            // glDrawElements(GL_TRIANGLES, water_index_count, GL_UNSIGNED_INT, 0);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            glEndQuery(GL_TIME_ELAPSED);

            glBeginQuery(GL_TIME_ELAPSED, mesh_queries[current_query_frame]);

            /*
                MESH PASS
                Render Target:
                    MAIN_TEXTURE on MAIN_FBO
                Using:
                    MESH_VAO
                    MESH_VBO
                    MESH_SHADER
                Desc:
            */
        
            glBindFramebuffer(GL_FRAMEBUFFER, MAIN_FBO);
            glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            if (GLOBALS.planet_meshing_toggle)
                {
            glUseProgram(MESH_SHADER.program);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, PERLIN_TEXTURE);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ROCK_NORMAL_TEXTURE);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, ROCK_DIFFUSE_TEXTURE);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, ROCK_SPECULAR_TEXTURE);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ROCK_AO_TEXTURE);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, SHADOW_MAP_TEXTURE);

            glUniform1i(MESH_SHADER.uniforms[0].location, GLOBALS.selected_body);
            // glUniform1i(glGetUniformLocation(MESH_SHADER, "uSelected"), 1);
            glUniform3f(MESH_SHADER.uniforms[1].location, cam.cam_data.cam_pos.x,
                        cam.cam_data.cam_pos.y, cam.cam_data.cam_pos.z);
            glUniform1f(MESH_SHADER.uniforms[2].location, uTime);
            glUniform1i(MESH_SHADER.uniforms[3].location, 0);
            glUniform1i(MESH_SHADER.uniforms[4].location, 1);
            glUniform1i(MESH_SHADER.uniforms[5].location, 2);
            glUniform1i(MESH_SHADER.uniforms[6].location, 3);
            glUniform1i(MESH_SHADER.uniforms[7].location, 4);
            glUniform1i(MESH_SHADER.uniforms[8].location, 5);
            glUniformMatrix4fv(
                MESH_SHADER.uniforms[9].location,
                1,
                GL_FALSE,
                (GLfloat *)&LIGHT_CAM.cam_data.proj
            );
            glUniformMatrix4fv(
                MESH_SHADER.uniforms[10].location,
                1,
                GL_FALSE,
                (GLfloat *)&LIGHT_CAM.cam_data.view
            );
            glUniform1f(glGetUniformLocation(MESH_SHADER.program, "uPlanetRadius"),
                planet_radius
            );

            render_mesh_planet();
        }
        glEndQuery(GL_TIME_ELAPSED);
        glDisable(GL_CULL_FACE);


        glBeginQuery(GL_TIME_ELAPSED, water_queries[current_query_frame]);
        /*
            WATER PASS
            Render Target:
                COMPOSITE_TEXTURE on COMPOSITE_FBO
            Using:
                WATER_VAO
                WATER_VBO
                WATER_SHADER
            Desc:
        */
        
        glBindFramebuffer(GL_FRAMEBUFFER, COMPOSITE_FBO);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        if (GLOBALS.water_toggle)
        {
            
            glBindVertexArray(WATER_VAO);
            glUseProgram(WATER_SHADER.program);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, CUBEMAP_TEXTURE);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, WATER_FFT_PONG_TEXTURE);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, WATER_FFT_PONG_TEXTURE_2);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, SKY_TEXTURE);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_CUBE_MAP, SKY_CUBEMAP);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, DEPTH_TEXTURE);
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, MAIN_TEXTURE);
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_2D, EXP_LOOKUP_TEXTURE);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_3D, FBM_TEXTURE);
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, SHADOW_MAP_TEXTURE);

            glBindBuffer(GL_ARRAY_BUFFER, WATER_VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WATER_EBO);
            glUniform1f(WATER_SHADER.uniforms[0].location, uTime);
            glUniform1i(WATER_SHADER.uniforms[1].location, 1);
            glUniform1i(WATER_SHADER.uniforms[2].location, 2);
            glUniform1i(WATER_SHADER.uniforms[3].location, 0);
            glUniform1i(WATER_SHADER.uniforms[4].location, 3);
            glUniform1i(WATER_SHADER.uniforms[5].location, 5);
            glUniform1i(WATER_SHADER.uniforms[6].location, 4);
            glUniform1i(glGetUniformLocation(WATER_SHADER.program, "uCubemap"), 6);
            glUniform1i(glGetUniformLocation(WATER_SHADER.program, "uDepth"), 7);
            glUniform1i(glGetUniformLocation(WATER_SHADER.program, "uScene"), 8);
            glUniform1i(glGetUniformLocation(WATER_SHADER.program, "uExp"), 9);
            glUniform1f(glGetUniformLocation(WATER_SHADER.program, "uPlanetRadius"),
                        planet_radius);
            glUniform1i(glGetUniformLocation(WATER_SHADER.program, "fbm_tex"), 10);
            glUniform1i(WATER_SHADER.uniforms[7].location, 11);
            glUniformMatrix4fv(WATER_SHADER.uniforms[8].location, 1, GL_FALSE,
                               (GLfloat *)&LIGHT_CAM.cam_data.proj);
            glUniformMatrix4fv(WATER_SHADER.uniforms[9].location, 1, GL_FALSE,
                               (GLfloat *)&LIGHT_CAM.cam_data.view);

            glDrawElements(GL_TRIANGLES, water_index_count, GL_UNSIGNED_INT, 0);
        }
        glEndQuery(GL_TIME_ELAPSED);

    }


    
    /*
        POSTPROCESS PASS
        Render Target:
            DEFAULT_TEXTURE on DEFAULT_FBO
        Using:
            MAIN_VAO
            MAIN_VBO
            MAIN_SHADER
        Desc:
    */
    glBeginQuery(GL_TIME_ELAPSED, postprocess_queries[current_query_frame]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(POSTPROCESS_SHADER.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, MAIN_TEXTURE);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ID_TEXTURE);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, DEPTH_TEXTURE);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, BLOOM_TEXTURE);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, BLOOM2_TEXTURE);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, HISTORY_TEXTURE[cur_history]);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, SHADOW_MAP_TEXTURE);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, COMPOSITE_TEXTURE);

    glUniform1i(POSTPROCESS_SHADER.uniforms[0].location, 0);
    glUniform1i(POSTPROCESS_SHADER.uniforms[1].location, 1);
    glUniform1i(POSTPROCESS_SHADER.uniforms[2].location, 2);
    glUniform1i(POSTPROCESS_SHADER.uniforms[3].location, 3);
    glUniform1i(POSTPROCESS_SHADER.uniforms[4].location, 4);
    glUniform1i(POSTPROCESS_SHADER.uniforms[5].location, 5);
    glUniform1i(POSTPROCESS_SHADER.uniforms[6].location, 6);
    glUniform1i(glGetUniformLocation(POSTPROCESS_SHADER.program, "uWater"), 7);

    glUniform1i(POSTPROCESS_SHADER.uniforms[7].location, GLOBALS.hovered_body);
    glUniform1i(POSTPROCESS_SHADER.uniforms[8].location, GLOBALS.selected_body);
    glUniform2f(POSTPROCESS_SHADER.uniforms[9].location,
        1.0 / SCREEN_WIDTH,
        1.0 / SCREEN_HEIGHT
    );
    glUniform1i(glGetUniformLocation(POSTPROCESS_SHADER.program, "outside_atmosphere"),
        !within_planet_atmosphere
    );

    glUniform3f(
        POSTPROCESS_SHADER.uniforms[10].location,
        cam.right.x, cam.right.y, cam.right.z
    );
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glEndQuery(GL_TIME_ELAPSED);


}

GLuint64 density_time = 0;
GLuint64 mc_time = 0;




int main(int argc, char *argv[]) {

    SDL_Window *window = NULL;
    SDL_GLContext context;
    NK_UNUSED(argc);
    NK_UNUSED(argv);

    // omp_set_num_threads(8);
    if (!init(&window, &context))
        return -1;


    ATMOSPHERE_VERTS = calloc(ATMOSPHERE_RES * ATMOSPHERE_RES * 6 * 10,
                              sizeof(AtmosphereVertex));
    ATMOSPHERE_INDICES =
        calloc(ATMOSPHERE_RES * ATMOSPHERE_RES * 6 * 10, sizeof(uint32_t));

    srand(time(NULL));
    
    running = 1;
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // init_random();
    // init_chunk_data();


    // Initializes the atmosphere spherical mesh

    ATMOSPHERE_VCNT = 0;
    ATMOSPHERE_ICNT = 0;
    
    mesh_atmosphere_sphere();

    glBindVertexArray(ATMOSPHERE_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, ATMOSPHERE_VBO);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        ATMOSPHERE_VCNT * sizeof(AtmosphereVertex),
        ATMOSPHERE_VERTS
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ATMOSPHERE_EBO);
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        0,
        ATMOSPHERE_ICNT * sizeof(uint32_t),
        ATMOSPHERE_INDICES
    );

    glGenQueries(QUERY_FRAMES, main_queries);
    glGenQueries(QUERY_FRAMES, bounds_queries);
    glGenQueries(QUERY_FRAMES, morton_queries);
    glGenQueries(QUERY_FRAMES, radix_queries);
    glGenQueries(QUERY_FRAMES, treecount_queries);
    glGenQueries(QUERY_FRAMES, tree_queries);
    glGenQueries(QUERY_FRAMES, com_queries);
    glGenQueries(QUERY_FRAMES, atmosphere_queries);
    glGenQueries(QUERY_FRAMES, clouds_queries);
    glGenQueries(QUERY_FRAMES, water_gen_queries);
    glGenQueries(QUERY_FRAMES, water_queries);
    glGenQueries(QUERY_FRAMES, mesh_generation_queries);
    glGenQueries(QUERY_FRAMES, mesh_queries);
    glGenQueries(QUERY_FRAMES, draw_queries);
    glGenQueries(QUERY_FRAMES, postprocess_queries);

    // upload particle data once


    glBindBuffer(GL_ARRAY_BUFFER, MAIN_VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        BODY_COUNT * sizeof(ParticleVertex),
        verts,
        GL_DYNAMIC_DRAW
    );


    Timer master, mesh, frame;

    cam.cam_data.cam_pos = (vec4){0, 0, 0, 0};
    cam.cam_data.cam_pos.z = 1005;
    sim_step();

    Timer col;

    space_cam = cam;

    while (running) {

        vec3 cam_pos = (vec3){
            current_cam->cam_data.cam_pos.x,
            current_cam->cam_data.cam_pos.y,
            current_cam->cam_data.cam_pos.z
        };

        within_planet_atmosphere =
            (cam_pos.z - planet_origin.z) <= planet_radius + 1000;
        // within_planet_atmosphere = false;

        if (within_planet_atmosphere) {
            current_cam = &cam;
        } else {
            current_cam = &space_cam;
        }

        frame_available ^= 2;
        // Timer top, bottom;
        start_timer(&master);
        density_time = 0;
        mc_time = 0;
        // now = SDL_GetTicks() / 1000.0f;
        // dt = now - last;
        // if (dt > 0.25) dt = 0.25;
        // last = now;
        
        /**<This value is hardcoded since our values are touchy.*/
        dt = 2;
        uTime += dt;

        // if (GLOBALS.lock_on)
        // {
        //     look_at_selected_body();
        // }
        // if (within_planet_atmosphere)
        // {
        //     load_chunks();
        // }

        start_timer(&mesh);
        if (within_planet_atmosphere && !halt_chunks && GLOBALS.planet_meshing_toggle) {

            glBeginQuery(GL_TIME_ELAPSED, mesh_generation_queries[current_query_frame]);
            load_chunks(planet_origin, planet_radius);
            glEndQuery(GL_TIME_ELAPSED);

        }
        double mesh_time = end_timer(&mesh);


        start_timer(&col);
        process_input(&window);

        if (within_planet_atmosphere) {

            space_cam.cam_data.cam_pos.z = cam.cam_data.cam_pos.z;
            space_cam.cam_data.cam_vel = cam.cam_data.cam_vel;
            space_cam.pitch = cam.pitch;
            space_cam.yaw = cam.yaw;
            space_cam.roll = cam.roll;

        } else {

            cam = space_cam;

        }
        
        update_camera(&cam);
        update_camera(&space_cam);
        update_light_camera(&LIGHT_CAM);

        double end = end_timer(&col);

        if (!within_planet_atmosphere && GLOBALS.particle_toggle) {
            sim_step();
        }
        start_timer(&frame);
        if (GLOBALS.water_toggle)
        {
        
            glBeginQuery(GL_TIME_ELAPSED, water_gen_queries[current_query_frame]);
     
            update_water_fft(wt);

            glEndQuery(GL_TIME_ELAPSED);
        }
        render();
        if (display_menu) {

            display_main_menu();
            nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        }
        SDL_GL_SwapWindow(window);
        double frame_time = end_timer(&frame);


        int oldest_frame = (current_query_frame + 1) % QUERY_FRAMES;        
        GLuint64 query_s[PQUERY_MAX];

        query_s[PQUERY_MAIN] = retrieve_gl_query(main_queries[oldest_frame]);
        query_s[PQUERY_BOUNDS] = retrieve_gl_query(bounds_queries[oldest_frame]);
        query_s[PQUERY_MORTON] = retrieve_gl_query(morton_queries[oldest_frame]);
        query_s[PQUERY_RADIX] = retrieve_gl_query(radix_queries[oldest_frame]);
        query_s[PQUERY_TREECOUNT] = retrieve_gl_query(treecount_queries[oldest_frame]);
        query_s[PQUERY_TREE] = retrieve_gl_query(tree_queries[oldest_frame]);
        query_s[PQUERY_COM] = retrieve_gl_query(com_queries[oldest_frame]);
        query_s[PQUERY_ATMOSPHERE] = retrieve_gl_query(atmosphere_queries[oldest_frame]);
        query_s[PQUERY_CLOUDS] = retrieve_gl_query(clouds_queries[oldest_frame]);
        query_s[PQUERY_WATER_GEN] = retrieve_gl_query(water_gen_queries[oldest_frame]);
        query_s[PQUERY_WATER] = retrieve_gl_query(water_queries[oldest_frame]);
        query_s[PQUERY_MESH_GEN] = retrieve_gl_query(mesh_generation_queries[oldest_frame]);
        query_s[PQUERY_PARTICLE] = retrieve_gl_query(particle_queries[oldest_frame]);
        query_s[PQUERY_MESH] = retrieve_gl_query(mesh_queries[oldest_frame]);
        query_s[PQUERY_DRAW] = retrieve_gl_query(draw_queries[oldest_frame]);
        query_s[PQUERY_POSTPROCESS] = retrieve_gl_query(postprocess_queries[oldest_frame]);

        double m = end_timer(&master);
        if (GLOBALS.profiling)
        {
            printf("CPU: COLLISION: %f MESH: %f FRAME: %f FULL: %f\n", end,
                   mesh_time, frame_time, m);

            
            printf("GPU time: Bounds %.3f ms | Morton %.3f ms | Radix %.3f ms | TC "
                   "%.3f ms | Tree %.3f ms | COM %.3f ms | Main %.3f ms \n Atmosphere "
                   "%.3f ms | Clouds %.3f ms | Water Gen %.3f | Mesh Gen %.3f \n Particle %.3f | Water %.3f | Shadowmap %.3f Mesh %.3f Postprocess %.3f\n",
                   query_s[PQUERY_BOUNDS] / 1e6,
                   query_s[PQUERY_MORTON] / 1e6,
                   query_s[PQUERY_RADIX] / 1e6,
                   query_s[PQUERY_TREECOUNT] / 1e6,
                   query_s[PQUERY_TREE] / 1e6,
                   query_s[PQUERY_COM] / 1e6,
                   query_s[PQUERY_MAIN] / 1e6,
                   query_s[PQUERY_ATMOSPHERE] / 1e6,
                   query_s[PQUERY_CLOUDS] / 1e6,
                   query_s[PQUERY_WATER_GEN] / 1e6,
                   query_s[PQUERY_MESH_GEN] / 1e6,
                   query_s[PQUERY_PARTICLE] / 1e6,
                   query_s[PQUERY_WATER] / 1e6,
                   query_s[PQUERY_DRAW] / 1e6,
                   query_s[PQUERY_MESH] / 1e6,
                   query_s[PQUERY_POSTPROCESS] / 1e6);
                   current_query_frame = (current_query_frame + 1) % QUERY_FRAMES;
        }

    }

    nk_sdl_shutdown();
    SDL_GL_DeleteContext(context);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 1;
}
