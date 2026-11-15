#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_video.h>
#include "utilities/mathf.h"
#include <math.h>
#include <stdbool.h>
#include "core/init.h"
#include "core/types.h"
#include "mesh/mesh.h"
#include "core/shaders.h"
#include "mesh/marching_cubes.h"
#include "stb/stb_image.h"
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"

GLuint
    load_texture
    (
        const char * path,
        GLenum       iformat,
        GLenum       format,
        GLenum       type
    )

{

    int w, h, channels;

    unsigned char *data = stbi_load(path, &w, &h, &channels, 0);

    if (!data) {
        printf("Failed to load image: %s\\n", path);
        return 0;
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, iformat, w, h, 0, format, type, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
    return textureID;

}

GLuint
    load_cubemap
    ( const char * path )

{
    const char * a[6] =
    {
        "/px.png", "/nx.png", "/py.png", "/ny.png", "/pz.png", "/nz.png"
    };
    
    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    for (int i = 0; i < 6; i++)
    {

        char c[428];
        strcpy(c, path);
        strcat(c, a[i]);

        int w, h, channels;

        unsigned char *data = stbi_load(c, &w, &h, &channels, 0);

        if (!data) {
            printf("Failed to load image: %s\\n", path);
            return 0;
        }

        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        
        
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
    
    return cubemap;

}

// Distribution for particle mass
float power_law(float min_m, float max_m, float alpha) {

    float r = (float)rand() / RAND_MAX;
    float a = 1.0f - alpha;

    return pow(r * (pow(max_m, a) - pow(min_m, a)) + pow(min_m, a), 1.0f / a);
}

// Complete mass after random distribution
float MASS = 0.0f;
void
    init_random
    ()

{

    memset(&PD, 0, sizeof(ParticleData));

    vec3 axis = (vec3){0, 0, 1};
    for (int i = 0; i < BODY_COUNT; i++) {

        float u = (float)rand() / RAND_MAX; 
        float v = (float)rand() / RAND_MAX; 

        float theta = 2.0f * M_PI * u;
        float z = 2.0f * v - 1.0f;

        float r = sqrt(1.0f - z * z);
        float radius = cbrtf((float)rand() / RAND_MAX) * GLOBALS.radius;

        vec3 p;
        p.x = r * cosf(theta) * radius;
        p.y = r * sinf(theta) * radius;
        p.z = z * radius;
        
        if (i == 0) {

            PD.p[i].p_m = (vec4){0, 0, 0, GLOBALS.max_mass};
            PD.p[i].vel = (vec4){0, 0, 0, 0};
            PD.p[i].acc = (vec4){0, 0, 0, 0};

        } else {

            PD.p[i].p_m = (vec4){p.x, p.y, p.z, power_law(GLOBALS.min_mass, GLOBALS.max_mass, GLOBALS.mass_pow)};
            

            vec3 tan = vec3_cross( axis, (vec3){PD.p[i].p_m.x, PD.p[i].p_m.y, PD.p[i].p_m.z});

            vec3 v2 = vec3_scale(vec3_normalize(tan), 0.5f);
            PD.p[i].vel = (vec4){v2.x, v2.y, v2.z, 0};
            PD.p[i].acc = (vec4){0, 0, 0, 0};
             
        }

        PD.p[i].mat =
            (vec4){(float)rand() / RAND_MAX, (float)rand() / RAND_MAX,
                   (float)rand() / RAND_MAX, (float)rand() / RAND_MAX};
    }
    for (int i = 0; i < BODY_COUNT; i++) {
        MASS += PD.p[i].p_m.w;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, PARTICLE_SSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ParticleData), &PD);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, PARTICLE_SSBO);

}
struct nk_context * NK_CTX = NULL;
bool NUKLEAR_HOVERED = false; 

vec4 * water_fft_arr = NULL;
float * phi = NULL;
// Generates a random number with a random distribution for water FFT constants.
float
    rand_normal ()
    
{
    
    float u1 = ((float)rand() + 1.0f) / (RAND_MAX + 1.0f);
    float u2 = ((float)rand() + 1.0f) / (RAND_MAX + 1.0f);
    return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);

}

WaterData WATER_DATA;

// Initializes the water grid with hardcoded constants.
// TODO make this changeable in real time.
/**
 * @brief Initializes the water grid at the start of the simulation with a wind direction.
 * @ingroup water_group
 */
void
    init_water_grid
    (
        int   N,
        int   L,
        float A
    )

{

    float PI_2 = M_PI * 2;
    vec2 w = {-5.0f, 12.0f};
    float w_magnitude = sqrtf(w.x * w.x + w.y * w.y);
    vec2 w_dir = {w.x / w_magnitude, w.y / w_magnitude};

    water_fft_arr = malloc(sizeof(vec4) * N * N);
    memset(WATER_DATA.water_data, 0, sizeof(vec4) * N * N);
    phi = malloc(sizeof(float) * N * N);

    int count = 0;
    for (int n = 0; n < N; n++)
    {
        for (int m = 0; m < N; m++)
        {
            float kx = (n <= N/2) ? n : n - N;
            float kz = (m <= N/2) ? m : m - N;

            kx *= (2.0f * M_PI / L);
            kz *= (2.0f * M_PI / L);
            float k_magnitude = sqrtf(kx * kx + kz * kz);
            int idx = n * N + m;
            if (k_magnitude < 1e-6f)
            {
                water_fft_arr[idx] = (vec4){0,0,0,0};
                count++;
                continue;
            }
            vec2 dir = {kx, kz};
            dir.x /= k_magnitude;
            dir.y /= k_magnitude;

            float k4 = pow(k_magnitude, 4.0f);

            float kw_dot = vec2_dot(dir, w_dir);
            kw_dot = pow(kw_dot, 2.0f);
            float damping = exp(-k_magnitude * k_magnitude * L * L * 0.001f);
            float p = A * (exp(-1/((k_magnitude * L) * (k_magnitude * L))) / k4) * kw_dot /* * damping */;

            WATER_DATA.water_data[idx].x = 1 / sqrtf(2.0f) * rand_normal() * sqrtf(p);
            WATER_DATA.water_data[idx].y = 1 / sqrtf(2.0f) * rand_normal() * sqrtf(p);
            WATER_DATA.water_data[idx].z =  WATER_DATA.water_data[idx].x;
            WATER_DATA.water_data[idx].w = -WATER_DATA.water_data[idx].y;
            count++;
        }
        
    }
    
}


float
    angular_velocity
    (
        vec2  pos,
        vec2  w,
        float len2
    )
{ return ((pos.x * w.y - pos.y * w.x) / len2); }

vec3 * out_arr = NULL;
GLuint * CURRENT_WATER_FFT_TEXTURE = NULL;

// FFTs the water texture, bit reversal first.
// output always in po
/**
 * @brief Performs a bit reversal and FFTs the water group. Output is in po.
 * @ingroup water_group
 */
void
    fft_textures
    (
        GLuint pi,
        GLuint po
    )
    
{
    
    int stages = log2(WATER_FFT_TEXTURE_SIZE);
    glUseProgram(BIT_REVERSAL_COMPUTE_SHADER);

    glUniform1i(glGetUniformLocation(BIT_REVERSAL_COMPUTE_SHADER, "log2N"), stages);
    glBindImageTexture(0, pi, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, po, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute((WATER_FFT_TEXTURE_SIZE+15) / 16, (WATER_FFT_TEXTURE_SIZE+15) / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    
    glUseProgram(WATER_FFT_COMPUTE_SHADER);
    GLuint * ping = &po;
    GLuint * pong = &pi;
    for (int d = 0; d < 2; d++)
    {
        for (int stage = 0; stage < stages; stage++)
        {
            glUniform1i(glGetUniformLocation(WATER_FFT_COMPUTE_SHADER, "direction"), d);
            glUniform1i(glGetUniformLocation(WATER_FFT_COMPUTE_SHADER, "stage"), stage);
            glUniform1i(glGetUniformLocation(WATER_FFT_COMPUTE_SHADER, "final_stage"), (stage == stages - 1 && d == 1) ? 1 : 0);

            glBindImageTexture(0, *ping, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(1, *pong, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

            glDispatchCompute((WATER_FFT_TEXTURE_SIZE+15) / 16, (WATER_FFT_TEXTURE_SIZE+15) / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            GLuint * temp = pong;
            pong = ping;
            ping = temp;
        }
    }
}


/**
 * @brief Updates the water grid and FFTs it once per frame.
 * @ingroup water_group
 * @see fft_textures()
 */
void
    update_water_fft
    (
        float t
    )

{

    glUseProgram(WATER_COMPUTE_SHADER);
    glBindImageTexture(0, WATER_FFT_PING_TEXTURE, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(1, WATER_FFT_PING_TEXTURE_2, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glUniform1f(glGetUniformLocation(WATER_COMPUTE_SHADER, "g"), 9.81f);
    glUniform1f(glGetUniformLocation(WATER_COMPUTE_SHADER, "t"), t);

    glDispatchCompute((WATER_FFT_TEXTURE_SIZE+15) / 16, (WATER_FFT_TEXTURE_SIZE+15) / 16, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    fft_textures(WATER_FFT_PING_TEXTURE, WATER_FFT_PONG_TEXTURE);
    fft_textures(WATER_FFT_PING_TEXTURE_2, WATER_FFT_PONG_TEXTURE_2);
}


// Noise pregen:
// FBM 
float * fbm_tex = NULL;


void
    precompute_fbm_texture ()
    
{
    
    fbm_tex = calloc(FBM_TEXTURE_SIZE * FBM_TEXTURE_SIZE * FBM_TEXTURE_SIZE, sizeof(float));

    for (int z = 0; z < FBM_TEXTURE_SIZE; z++)
    {
        
        for (int y = 0; y < FBM_TEXTURE_SIZE; y++)
        {
        
            for (int x = 0; x < FBM_TEXTURE_SIZE; x++)
            {
                vec3 p = {(float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f};
                float d = fbm(p);

                fbm_tex[z*FBM_TEXTURE_SIZE*FBM_TEXTURE_SIZE+y*FBM_TEXTURE_SIZE+x] = (d + 1) / 2;
            }
        }
    }
    
    glBindTexture(GL_TEXTURE_3D, FBM_TEXTURE);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, FBM_TEXTURE_SIZE, FBM_TEXTURE_SIZE, FBM_TEXTURE_SIZE, GL_RED, GL_FLOAT, fbm_tex);
}

// Worley
vec4 * worley_tex = NULL;

vec3
    random3
    ( vec3 p )

{
    vec3 s = (vec3){
        vec3_dot(p, (vec3){127.1,311.7,473.4}),
        vec3_dot(p, (vec3){446.1,269.5,183.3}),
        vec3_dot(p, (vec3){232.5,460.2,124.2})
    };

    s = (vec3){sinf(s.x), sinf(s.y), sinf(s.z)};
    s = vec3_scale(s, 43758.5453);
    s.x -= floorf(s.x);
    s.y -= floorf(s.y);
    s.z -= floorf(s.z);
    
    return s;
}

float
    worley
    ( vec3 p )

{

    vec3 fp = (vec3){floorf(p.x), floorf(p.y), floorf(p.z)};
    vec3 frp = (vec3){p.x - fp.x, p.y - fp.y, p.z - fp.z};

    float m_dist = 1;
    
    for (int z = -1; z <= 1; z++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {

                vec3 neighbor = (vec3){x, y, z};
                vec3 point = random3(vec3_add(fp, neighbor));
                vec3 diff = vec3_sub(vec3_add(neighbor, point), frp);

                float dist = vec3_length(diff);
                m_dist = MIN(m_dist, dist);

            }
        }
    }
    
    return m_dist;

}


void
    precompute_worley_texture ()
{
    worley_tex = calloc(WORLEY_TEXTURE_SIZE * WORLEY_TEXTURE_SIZE * WORLEY_TEXTURE_SIZE, sizeof(vec4));

    const float f0 = 0.05f;
    const float f1 = 0.51;
    const float f2 = 1.05f;
    const float f3 = 2.1f;

    for (int z = 0; z < WORLEY_TEXTURE_SIZE; z++)
    {
        
        for (int y = 0; y < WORLEY_TEXTURE_SIZE; y++)
        {
        
            for (int x = 0; x < WORLEY_TEXTURE_SIZE; x++)
            {
                vec3 p = {(float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f};
                vec3 p0 = vec3_scale(p, f0);
                vec3 p1 = vec3_scale(p, f1);
                vec3 p2 = vec3_scale(p, f2);
                vec3 p3 = vec3_scale(p, f3);


                vec4 d = (vec4){worley(p0), worley(p1), worley(p2), worley(p3)};
                
                worley_tex[z*WORLEY_TEXTURE_SIZE*WORLEY_TEXTURE_SIZE+y*WORLEY_TEXTURE_SIZE+x] = d;
            }
        }
    }
    
    glBindTexture(GL_TEXTURE_3D, WORLEY_TEXTURE);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, WORLEY_TEXTURE_SIZE, WORLEY_TEXTURE_SIZE, WORLEY_TEXTURE_SIZE, GL_RGBA, GL_FLOAT, worley_tex);

}


// Perlin
float fade(float t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

int permutations[PERLIN_TEXTURE_SIZE];

void
    make_perm ()

{

    for (int i = 0; i < PERLIN_TEXTURE_SIZE / 2; i++) {
        permutations[i] = i;
    }

    for (int i = PERLIN_TEXTURE_SIZE / 2 - 1; i > 0; i--) {
        int index = rand() % (PERLIN_TEXTURE_SIZE / 2 - 1);
        int temp = permutations[i];
        permutations[i] = permutations[index];
        permutations[index] = temp;
    }

    for (int i = PERLIN_TEXTURE_SIZE / 2; i < PERLIN_TEXTURE_SIZE; i++) {
        permutations[i] = permutations[i - PERLIN_TEXTURE_SIZE / 2];
    }
}

vec3 gradients[12] = {{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
                      {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
                      {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}};

float
    wrap_256
    ( float t )
{ return t - (floor(t) / 256) * 256; }

float
    noise3d
    ( vec3 p )
{
    int pm1 = PERLIN_TEXTURE_SIZE - 1;

    vec3 pw = {fmodf(p.x, PERLIN_TEXTURE_SIZE), fmodf(p.y, PERLIN_TEXTURE_SIZE),
               fmodf(p.z, PERLIN_TEXTURE_SIZE)};
    ivec3 p0 = (ivec3){(int)floorf(pw.x), (int)floorf(pw.y), (int)floorf(pw.z)};

    int px1 = (int)(p0.x + 1) & pm1;
    int py1 = (int)(p0.y + 1) & pm1;
    int pz1 = (int)(p0.z + 1) & pm1;

    float pmx = pw.x - px1;
    if (fabsf(pmx) > 1) {
        pmx = (pw.x - pm1) - px1;
    }
    float pmy = pw.y - py1;
    if (fabsf(pmy) > 1) {
        pmy = (pw.y - pm1) - py1;
    }
    float pmz = pw.z - pz1;
    if (fabsf(pmz) > 1) {
        pmz = (pw.z - pm1) - pz1;
    }

    ivec3 p1 = {px1, p0.y, p0.z};
    ivec3 p2 = {p0.x, py1, p0.z};
    ivec3 p3 = {px1, py1, p0.z};
    ivec3 p4 = {p0.x, p0.y, pz1};
    ivec3 p5 = {px1, p0.y, pz1};
    ivec3 p6 = {p0.x, py1, pz1};
    ivec3 p7 = {px1, py1, pz1};

    int e0 =
        permutations[(permutations[(permutations[p0.x] + p0.y) & pm1] + p0.z) &
                     pm1];
    int e1 =
        permutations[(permutations[(permutations[p1.x] + p1.y) & pm1] + p1.z) &
                     pm1];
    int e2 =
        permutations[(permutations[(permutations[p2.x] + p2.y) & pm1] + p2.z) &
                     pm1];
    int e3 =
        permutations[(permutations[(permutations[p3.x] + p3.y) & pm1] + p3.z) &
                     pm1];
    int e4 =
        permutations[(permutations[(permutations[p4.x] + p4.y) & pm1] + p4.z) &
                     pm1];
    int e5 =
        permutations[(permutations[(permutations[p5.x] + p5.y) & pm1] + p5.z) &
                     pm1];
    int e6 =
        permutations[(permutations[(permutations[p6.x] + p6.y) & pm1] + p6.z) &
                     pm1];
    int e7 =
        permutations[(permutations[(permutations[p7.x] + p7.y) & pm1] + p7.z) &
                     pm1];

    vec3 g0 = gradients[e0 % 12];
    vec3 g1 = gradients[e1 % 12];
    vec3 g2 = gradients[e2 % 12];
    vec3 g3 = gradients[e3 % 12];
    vec3 g4 = gradients[e4 % 12];
    vec3 g5 = gradients[e5 % 12];
    vec3 g6 = gradients[e6 % 12];
    vec3 g7 = gradients[e7 % 12];

    float t0 = pw.x - p0.x;
    float fade_t0 = fade(t0);

    float t1 = pw.y - p0.y;
    float fade_t1 = fade(t1);

    float t2 = pw.z - p0.z;
    float fade_t2 = fade(t2);
    // printf("T0 %f T1 %f T2 %f\n", t0, t1, t2);

    vec3 pp0 = {pw.x - p0.x, pw.y - p0.y, pw.z - p0.z};
    vec3 pp1 = {pmx, pw.y - p1.y, pw.z - p1.z};
    vec3 pp2 = {pw.x - p2.x, pmy, pw.z - p2.z};
    vec3 pp3 = {pmx, pmy, pw.z - p3.z};
    vec3 pp4 = {pw.x - p4.x, pw.y - p4.y, pmz};
    vec3 pp5 = {pmx, pw.y - p5.y, pmz};
    vec3 pp6 = {pw.x - p6.x, pmy, pmz};
    vec3 pp7 = {pmx, pmy, pmz};

    float p0p1 =
        (1.0 - fade_t0) * vec3_dot(g0, pp0) + fade_t0 * vec3_dot(g1, pp1);
    float p2p3 =
        (1.0 - fade_t0) * vec3_dot(g2, pp2) + fade_t0 * vec3_dot(g3, pp3);

    float p4p5 =
        (1.0 - fade_t0) * vec3_dot(g4, pp4) + fade_t0 * vec3_dot(g5, pp5);
    float p6p7 =
        (1.0 - fade_t0) * vec3_dot(g6, pp6) + fade_t0 * vec3_dot(g7, pp7);

    float y1 = (1.0 - fade_t1) * p0p1 + fade_t1 * p2p3;
    float y2 = (1.0 - fade_t1) * p4p5 + fade_t1 * p6p7;

    return (1.0 - fade_t2) * y1 + fade_t2 * y2;
}

float *perlin_tex;

void
    precompute_perlin_texture ()
{

    make_perm();
    const float FREQUENCY = 1.0;
    const float AMPLITUDE = 1.0;

    perlin_tex =
        calloc(PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE,
               sizeof(float));

    for (int z = 0; z < PERLIN_TEXTURE_SIZE; z++) {
        for (int y = 0; y < PERLIN_TEXTURE_SIZE; y++) {
            for (int x = 0; x < PERLIN_TEXTURE_SIZE; x++) {

                vec3 p = {(float)x + 0.5, (float)y + 0.5, (float)z + 0.5};

                float n = noise3d(vec3_scale(p, FREQUENCY)) * AMPLITUDE;

                perlin_tex[z * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE +
                           y * PERLIN_TEXTURE_SIZE + x] = (n + 1) / 2;
            }
        }
    }

    glBindTexture(GL_TEXTURE_3D, PERLIN_TEXTURE);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, PERLIN_TEXTURE_SIZE,
                    PERLIN_TEXTURE_SIZE, PERLIN_TEXTURE_SIZE, GL_RED, GL_FLOAT,
                    perlin_tex);
}

void init_globals()
{

    GLOBALS.min_mass = 1000;
    GLOBALS.max_mass = 1000;
    GLOBALS.mass_pow = 2.0f;
    GLOBALS.radius = 2000;
    GLOBALS.planet_meshing_toggle = 1;
    GLOBALS.water_toggle = 1;
    GLOBALS.particle_toggle = 1;
    GLOBALS.sky_toggle = 1;
    
}

bool
    init
    (
        SDL_Window    ** window,
        SDL_GLContext *  context
    )

{

    printf("Initializing\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL Init failed: %s\n", SDL_GetError());
        return false;
    }
    
    #ifdef INCLUDE_CONFIGURATOR
    static struct nk_color color_table[NK_COLOR_COUNT];
    memcpy(color_table, nk_default_color_style, sizeof(color_table));
    #endif

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetSwapInterval(1);
    BLOOM_WIDTH = SCREEN_WIDTH / 2;
    BLOOM_HEIGHT = SCREEN_HEIGHT / 2;
    BLOOM2_WIDTH = SCREEN_WIDTH / 4;
    BLOOM2_HEIGHT = SCREEN_HEIGHT / 4;

    *window = SDL_CreateWindow("Space", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);

    // SDL_SetWindowFullscreen(*window, SDL_WINDOW_FULLSCREEN);
    SDL_SetWindowGrab(*window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    if (!*window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    *context = SDL_GL_CreateContext(*window);
    if (!*context) {
        printf("GL Context failed: %s\n", SDL_GetError());
        return false;
    }

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("GLEW Init failed\n");
        return false;
    }

    NK_CTX = nk_sdl_init(*window);
    if (!NK_CTX)
    {
        printf("NUKLEAR INIT FAILED\n");
        return false;
    }
    {struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();
    }

    #ifdef INCLUDE_STYLE
    #ifdef STYLE_WHITE
    set_style(ctx, THEME_WHITE);
    #elif defined(STYLE_RED)
    set_style(ctx, THEME_RED);
    #elif defined(STYLE_BLUE)
    set_style(ctx, THEME_BLUE);
    #elif defined(STYLE_DARK)
    set_style(ctx, THEME_DARK);
    #endif
    #endif

    struct nk_color transparent = nk_rgba(30, 30, 30, 90);
    struct nk_color border = nk_rgba(60, 60, 60, 150);
    struct nk_color text = nk_rgba(255, 255, 255, 255);

    NK_CTX->style.window.fixed_background = nk_style_item_color(transparent);
    NK_CTX->style.window.border_color = border;
    NK_CTX->style.window.padding = nk_vec2(10, 10);
    NK_CTX->style.window.border = 1;

    NK_CTX->style.text.color = text;


    init_globals();
    
    printf("Textures\n");
    glGenTextures(1, &SPHERE_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, SPHERE_TEXTURE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 256, 256, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    init_water_grid(WATER_FFT_TEXTURE_SIZE, WATER_FFT_L, 2.0);

    glGenTextures(1, &WATER_FFT_PING_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, WATER_FFT_PING_TEXTURE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WATER_FFT_TEXTURE_SIZE, WATER_FFT_TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &WATER_FFT_PONG_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, WATER_FFT_PONG_TEXTURE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WATER_FFT_TEXTURE_SIZE, WATER_FFT_TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &WATER_FFT_PING_TEXTURE_2);
    glBindTexture(GL_TEXTURE_2D, WATER_FFT_PING_TEXTURE_2);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WATER_FFT_TEXTURE_SIZE, WATER_FFT_TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &WATER_FFT_PONG_TEXTURE_2);
    glBindTexture(GL_TEXTURE_2D, WATER_FFT_PONG_TEXTURE_2);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WATER_FFT_TEXTURE_SIZE, WATER_FFT_TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &WORLEY_TEXTURE);
    glBindTexture(GL_TEXTURE_3D, WORLEY_TEXTURE);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, WORLEY_TEXTURE_SIZE, WORLEY_TEXTURE_SIZE, WORLEY_TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    precompute_worley_texture();

    glGenTextures(1, &PERLIN_TEXTURE);
    glBindTexture(GL_TEXTURE_3D, PERLIN_TEXTURE);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, PERLIN_TEXTURE_SIZE, PERLIN_TEXTURE_SIZE, PERLIN_TEXTURE_SIZE, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    precompute_perlin_texture();

    glGenTextures(1, &FBM_TEXTURE);
    glBindTexture(GL_TEXTURE_3D, FBM_TEXTURE);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, FBM_TEXTURE_SIZE, FBM_TEXTURE_SIZE, FBM_TEXTURE_SIZE, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    precompute_fbm_texture();

    glGenTextures(1, &SKY_CUBEMAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, SKY_CUBEMAP);

    int sky_cubemap_size = 128;
    for (int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, sky_cubemap_size, sky_cubemap_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);

    ROCK_NORMAL_TEXTURE = load_texture("./textures/rocky_terrain_03_nor_gl_4k.png", GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
    ROCK_DIFFUSE_TEXTURE = load_texture("./textures/rocky_terrain_03_diff_4k.png", GL_SRGB8_ALPHA8, GL_RGB, GL_UNSIGNED_BYTE);
    
    ROCK_SPECULAR_TEXTURE = load_texture("./textures/rocky_terrain_03_spec_4k.png", GL_R8, GL_RED, GL_UNSIGNED_BYTE);
    ROCK_AO_TEXTURE = load_texture("./textures/rocky_terrain_03_ao_4k.png", GL_R8, GL_RED, GL_UNSIGNED_BYTE);

    CUBEMAP_TEXTURE = load_cubemap("./cubemap");

    glGenTextures(1, &MAIN_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, MAIN_TEXTURE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &ID_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, ID_TEXTURE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glGenTextures(1, &DEPTH_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, DEPTH_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glGenFramebuffers(1, &MAIN_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, MAIN_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, MAIN_TEXTURE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ID_TEXTURE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, DEPTH_TEXTURE, 0);

    glGenTextures(1, &COMPOSITE_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, COMPOSITE_TEXTURE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &COMPOSITE_DEPTH_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, COMPOSITE_DEPTH_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glGenFramebuffers(1, &COMPOSITE_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, COMPOSITE_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, COMPOSITE_TEXTURE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, COMPOSITE_DEPTH_TEXTURE, 0);

    // this is hilarious i am keeping it
    if (!glewIsSupported("GL_ARB_buffer_storage")) {
        printf("SHIT\n");
    }

    glGenTextures(1, &EXP_LOOKUP_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, EXP_LOOKUP_TEXTURE);


    float exp_lookup[1000];

    for (int i = 0; i < 1000; i++)
    {
        exp_lookup[i] = expf((float)-i / 50);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 1000, 1, 0, GL_RED, GL_FLOAT, exp_lookup);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    
    glGenFramebuffers(1, &SKY_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, SKY_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP, SKY_CUBEMAP, 0);

    glGenTextures(1, &SHADOW_MAP_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, SHADOW_MAP_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

    glGenFramebuffers(1, &SHADOW_MAP_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, SHADOW_MAP_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, SHADOW_MAP_TEXTURE, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

        
    glGenTextures(1, &POSTPROCESS_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, POSTPROCESS_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &POSTPROCESS_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, POSTPROCESS_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, POSTPROCESS_TEXTURE, 0);

    glGenTextures(1, &BLOOM_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, BLOOM_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, BLOOM_WIDTH, BLOOM_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &BLOOM_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, BLOOM_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BLOOM_TEXTURE, 0);

    glGenTextures(1, &BLUR_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, BLUR_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, BLOOM_WIDTH, BLOOM_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &BLUR_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, BLUR_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BLUR_TEXTURE, 0);

    glGenTextures(1, &BLOOM2_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, BLOOM2_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, BLOOM2_WIDTH, BLOOM2_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &BLOOM2_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, BLOOM2_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BLOOM2_TEXTURE, 0);

    glGenTextures(1, &BLUR2_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, BLUR2_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, BLOOM2_WIDTH, BLOOM2_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenFramebuffers(1, &BLUR2_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, BLUR2_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BLUR2_TEXTURE, 0);

    glGenTextures(1, &HISTORY_TEXTURE[0]);
    glBindTexture(GL_TEXTURE_2D, HISTORY_TEXTURE[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, BLOOM_WIDTH, BLOOM_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &HISTORY_FBO[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, HISTORY_FBO[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HISTORY_TEXTURE[0], 0);
    
    glGenTextures(1, &HISTORY_TEXTURE[1]);
    glBindTexture(GL_TEXTURE_2D, HISTORY_TEXTURE[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, BLOOM_WIDTH, BLOOM_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &HISTORY_FBO[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, HISTORY_FBO[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HISTORY_TEXTURE[1], 0);

    printf("SSBO\n");

    glGenBuffers(1, &CAM_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, CAM_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CamData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, CAM_UBO); 

    glGenBuffers(1, &SPACE_CAM_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, SPACE_CAM_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CamData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, SPACE_CAM_UBO); 

    glGenBuffers(1, &RENDER_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, RENDER_UBO);
    ivec4 res = (ivec4){SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0};
    glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderData), &res, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, RENDER_UBO); 

    glGenBuffers(1, &PARTICLE_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, PARTICLE_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticleData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, PARTICLE_SSBO); 

    glGenBuffers(1, &MORTON_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, MORTON_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(MortonData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, MORTON_SSBO); 

    glGenBuffers(1, &MORTON_PING_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, MORTON_PING_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(MortonData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, MORTON_PING_SSBO); 

    glGenBuffers(1, &NODE_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, NODE_SSBO);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(NodeData), NULL, GL_MAP_READ_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, NODE_SSBO); 

    glGenBuffers(1, &BOUNDS_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, BOUNDS_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BoundsData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, BOUNDS_SSBO); 

    glGenBuffers(1, &NODE_COUNTER_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, NODE_COUNTER_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(NodeCounter), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, NODE_COUNTER_SSBO); 

    glGenBuffers(1, &RADIX_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, RADIX_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(RadixData), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, RADIX_SSBO); 

    glGenBuffers(1, &MARCHING_CUBES_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, MARCHING_CUBES_SSBO);

    MarchingCubesData DD;
    memset(&DD, 0, sizeof(DD));
    memcpy(DD.EDGE_TO_VERTICES, EDGE_TO_VERTICES, sizeof(EDGE_TO_VERTICES));
    memcpy(DD.EDGE_TABLE, EDGE_TABLE, sizeof(EDGE_TABLE));
    memcpy(DD.TRIANGLE_TABLE, TRIANGLE_TABLE, sizeof(TRIANGLE_TABLE));
    
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(MarchingCubesData), &DD, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, MARCHING_CUBES_SSBO); 

    for (int i = 0; i < LODMAX; i++)
    {
        glGenBuffers(1, &DENSITY_SSBO[i]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, DENSITY_SSBO[i]);

        LOD_DENSITY_SIZES[i] = (LOD_RES[i]+1) * (LOD_RES[i]+1) * (LOD_RES[i]+1) * MAX_CHUNKS_LOD[i];
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * LOD_DENSITY_SIZES[i], NULL, GL_DYNAMIC_DRAW);


    }
    for (int i = 0; i < LODMAX; i++)
    {
        
        glGenBuffers(1, &DENSITY_VERTEX_SSBO[i]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, DENSITY_VERTEX_SSBO[i]);

        // 12 = number of edges, 6 = 2 quads
        LOD_VERTEX_SIZES[i] = (((LOD_RES[i]) * (LOD_RES[i]) * (LOD_RES[i]) * 15) + ((LOD_RES[i] + 2) * 12 * 6)) * MAX_CHUNKS_LOD[i];
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * MAX_CHUNKS + LOD_VERTEX_SIZES[i] * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
    }



    // This is sort of still a problem. Because of our drivers not supporting persistent mapping, we need to consistently map buffers to check for empty chunks.
    // There has to be some workaround for it but I haven't found it.
    for (int i = 0; i < 3; i++)
    {
        
        glGenBuffers(1, &CHUNK_DATA_SSBO[i]);
        GPUChunk chunk;
        memset(&chunk, -1, sizeof(chunk));
        chunk.lod_dist[0] = CHUNK_WORLD_SIZE * 2 - 1e-4;
        chunk.lod_dist[1] = CHUNK_WORLD_SIZE * 4 - 1e-4;
        chunk.lod_dist[2] = CHUNK_WORLD_SIZE * 6 - 1e-4;
        chunk.emitted_chunks = 0;


        glBindBuffer(GL_SHADER_STORAGE_BUFFER, CHUNK_DATA_SSBO[i]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(GPUChunk), &chunk, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);
        // glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GPUChunk), &chunk, GL_DYNAMIC_DRAW);
        // CHUNK_DATA_PTR[i] = glMapBufferRange(
        //     GL_SHADER_STORAGE_BUFFER,
        //     offsetof(GPUChunk, vertex_counts),
        //     sizeof(int) * LODMAX * MAX_CHUNKS,
        //     GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT
        // );

    }

    CHUNK_DATA_FLAG = 0;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, CHUNK_DATA_SSBO[CHUNK_DATA_FLAG]); 

    PLANET_DATA.sun_dir = (vec4){0.1, 0.2, -0.8, 0.0};
    PLANET_DATA.sun_color = (vec4){1.0, 0.7, 0.5, 1.0f};
    PLANET_DATA.warp_count = 0;
    PLANET_DATA.warp_layers[0].offset0 = (vec3){0, 0, 0};
    PLANET_DATA.warp_layers[0].freq0 = 0.001f;
    PLANET_DATA.warp_layers[0].offset1 = (vec3){1, 1, 1};
    PLANET_DATA.warp_layers[0].freq1 = 0.001f;
    PLANET_DATA.warp_layers[0].offset2 = (vec3){5, 5, 5};
    PLANET_DATA.warp_layers[0].freq2 = 0.001f;
    PLANET_DATA.warp_layers[0].amp = 0.0f;
    PLANET_DATA.perlin_count = 4;
    PLANET_DATA.perlin_layers[0].freq = 0.001f;
    PLANET_DATA.perlin_layers[0].amp = 50.0f;
    PLANET_DATA.perlin_layers[1].freq = 0.0025f;
    PLANET_DATA.perlin_layers[1].amp = 1.0f;
    PLANET_DATA.perlin_layers[2].freq = 0.0005f;
    PLANET_DATA.perlin_layers[2].amp = 5.0f;
    PLANET_DATA.perlin_layers[3].freq = 0.0001f;
    PLANET_DATA.perlin_layers[3].amp = 8.0f;
    PLANET_DATA.fbm_count = 1;
    PLANET_DATA.fbm_layers[0].freq = 0.0005f;
    PLANET_DATA.fbm_layers[0].amp = 10.0f;
    
    glGenBuffers(1, &PLANET_DATA_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, PLANET_DATA_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PlanetData), &PLANET_DATA, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, PLANET_DATA_SSBO); 

    glGenBuffers(1, &WATER_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, WATER_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(WaterData), &WATER_DATA, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, WATER_SSBO); 
    
    printf("Shaders\n");
    init_shaders();
    
    MAIN_COMPUTE_SHADER = create_compute_shader("./src/particles/main.comp");
    ZERO_COMPUTE_SHADER = create_compute_shader("./src/particles/zero.comp");
    CHUNK_ZERO_COMPUTE_SHADER = create_compute_shader("./src/mesh/chunk_zero.comp");
    BOUNDS_COMPUTE_SHADER = create_compute_shader("./src/particles/bounds.comp");
    MORTON_COMPUTE_SHADER = create_compute_shader("./src/particles/morton.comp");
    RADIX_COMPUTE_SHADER = create_compute_shader("./src/particles/radix_new.comp");
    RADIX_GLOBAL_COMPUTE_SHADER = create_compute_shader("./src/particles/radix_global.comp");
    RADIX_FINAL_COMPUTE_SHADER = create_compute_shader("./src/particles/radix_scatter.comp");
    TREE_COMPUTE_SHADER = create_compute_shader("./src/particles/tree.comp");
    TREE_COUNT_COMPUTE_SHADER = create_compute_shader("./src/particles/treecount.comp");
    COM_COMPUTE_SHADER = create_compute_shader("./src/particles/com22.comp");
    NEXT_COMPUTE_SHADER = create_compute_shader("./src/particles/next.comp");
    SPAWN_COMPUTE_SHADER = create_compute_shader("./src/particles/spawn.comp");
    POS_COMPUTE_SHADER = create_compute_shader("./src/particles/pos.comp");
    WATER_COMPUTE_SHADER = create_compute_shader("./src/FFT/water.comp");
    BIT_REVERSAL_COMPUTE_SHADER = create_compute_shader("./src/FFT/bit_reversal.comp");
    WATER_FFT_COMPUTE_SHADER = create_compute_shader("./src/FFT/fft.comp");

    printf("Vertices\n");
    glGenVertexArrays(1, &SKY_VAO);
    glBindVertexArray(SKY_VAO);
    glGenBuffers(1, &SKY_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, SKY_VBO);

    float skyboxVertices[] = {

        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f

    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 36 * 3, skyboxVertices, GL_STATIC_DRAW);

    
    // Skybox
    size_t sky_stride = sizeof(float) * 3;
    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, sky_stride, (void *)(uintptr_t)0);

    // Particle VAO
    glGenVertexArrays(1, &MAIN_VAO);
    glBindVertexArray(MAIN_VAO);
    glGenBuffers(1, &MAIN_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, MAIN_VBO);

    size_t main_stride = sizeof(float) * 6;
    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, main_stride, (void *)(uintptr_t)0);
    // vel
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, 0, main_stride, (void *)(uintptr_t)(sizeof(float) * 3));

    // Mesh VAO
    glGenVertexArrays(1, &MESH_VAO);
    glBindVertexArray(MESH_VAO);
    glGenBuffers(1, &MESH_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, MESH_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MESH_RES * MESH_RES * 6 * 10, NULL, GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &MESH_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MESH_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * MESH_RES * MESH_RES * 6 * 10, NULL, GL_DYNAMIC_DRAW);
    
    size_t mesh_stride = sizeof(float) * 9;
    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, mesh_stride, (void *)(uintptr_t)0);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, 0, mesh_stride, (void *)(uintptr_t)(sizeof(float) * 3));
    // color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, 0, mesh_stride, (void *)(uintptr_t)(sizeof(float) * 6));

    // Atmosphere VAO
    glGenVertexArrays(1, &ATMOSPHERE_VAO);
    glBindVertexArray(ATMOSPHERE_VAO);
    glGenBuffers(1, &ATMOSPHERE_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, ATMOSPHERE_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(AtmosphereVertex) * ATMOSPHERE_RES * ATMOSPHERE_RES * 6 * 10, NULL, GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &ATMOSPHERE_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ATMOSPHERE_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * ATMOSPHERE_RES * ATMOSPHERE_RES * 6 * 10, NULL, GL_DYNAMIC_DRAW);

    size_t atmosphere_stride = sizeof(float) * 6;
    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, atmosphere_stride, (void *)(uintptr_t)0);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, 0, atmosphere_stride, (void *)(uintptr_t)(sizeof(float) * 3));

    // Water VAO
    glGenVertexArrays(1, &WATER_VAO);
    glBindVertexArray(WATER_VAO);

    water_quad_count= (WATER_RES - 1) * (WATER_RES - 1);
    water_index_count= water_quad_count* 6;

    // Water Plane Vertex Generation
    // simply makes a horizontal plane so we can interpolate during runtime
    uint32_t* indices = malloc(sizeof(uint32_t) * water_index_count);
    int i = 0;
    WaterVertex  * vertices = malloc(sizeof(WaterVertex) * WATER_RES * WATER_RES);
    for (int y = 0; y < WATER_RES; y++)
    for (int x = 0; x < WATER_RES; x++)
    {

        float u = (x + 0.5f) / WATER_RES;
        float v = (y + 0.5f) / WATER_RES;

        vertices[y * WATER_RES + x].pos = (vec3){
            u - 0.5f,
            v - 0.5f,
            0.0f
        };
        
    }
    for (int y = 0; y < WATER_RES-1; y++)
    for (int x = 0; x < WATER_RES-1; x++)
    {

        int i0 = y * WATER_RES + x;
        int i1 = i0 + 1;
        int i2 = i0 + WATER_RES;
        int i3 = i2 + 1;

        indices[i++] = i0;
        indices[i++] = i2;
        indices[i++] = i1;

        indices[i++] = i1;
        indices[i++] = i2;
        indices[i++] = i3;
    }

    glGenBuffers(1, &WATER_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, WATER_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(WaterVertex) * WATER_RES * WATER_RES, vertices, GL_STATIC_DRAW);
    size_t water_stride = sizeof(float) * 3;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, water_stride, (void *)(uintptr_t)0);

    glGenBuffers(1, &WATER_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WATER_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * water_index_count, indices, GL_STATIC_DRAW);

    free(vertices);
    free(indices);

    // Chunk Indirect Buffers
    // Used to hold per-chunk vertices and data
    // for (int l = 0; l < LODMAX; l++)
    // {
    //     for (int i = 0; i < MAX_CHUNKS_LOD[l]; i++)
    //     {
    //         glGenBuffers(1, &INDIRECT_BUFFER[l][i]);

    //         glBindBuffer(GL_DRAW_INDIRECT_BUFFER, INDIRECT_BUFFER[l][i]);
    //         glBufferData(GL_DRAW_INDIRECT_BUFFER,
    //                      sizeof(DrawArraysIndirectCommand),
    //                      NULL,
    //                      GL_DYNAMIC_DRAW);

    //         CHUNK_DATA[l][i].free = true;
    //         CHUNK_DATA[l][i].is_child = false;
    //         CHUNK_DATA[l][i].uID = -1;
    //         CHUNK_DATA[l][i].id0 = -1;
    //         CHUNK_DATA[l][i].id1 = -1;
        
    //     }
        
    // }

    glGenVertexArrays(1, &FS_VAO);
    init_random();
    init_chunk_data();
    
    return true;
}
