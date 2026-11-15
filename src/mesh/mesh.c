
#include "core/init.h"
#include "utilities/mathf.h"
#include "core/types.h"
#include "mesh/mesh.h"
#include "uthash/uthash.h"
#include "core/camera.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <float.h>




typedef enum FACE
{
    XPOS,
    XNEG,
    YPOS,
    YNEG,
    ZPOS,
    ZNEG
    
} FACE;


/**
 * @ingroup atmosphere_group
 */
#define ATMOSPHERE_IDX(x, y, o) ((o) + (y) * ATMOSPHERE_RES + (x))



vec3
    cube_pos
    (
        FACE face,
        vec2 uv
    )

{
    
    switch(face) {

        case XPOS: return (vec3){ 1,  uv.y, -uv.x}; 
        case XNEG: return (vec3){-1,  uv.y,  uv.x}; 
        case YPOS: return (vec3){ uv.x,  1, -uv.y}; 
        case YNEG: return (vec3){ uv.x, -1,  uv.y}; 
        case ZPOS: return (vec3){ uv.x,  uv.y,  1}; 
        case ZNEG: return (vec3){-uv.x,  uv.y, -1}; 

    }

}

vec3
    calc_normal
    (
        vec3 a,
        vec3 b,
        vec3 c
    )

{

	vec3 u = (vec3){b.x - a.x, b.y - a.y, b.z - a.z};
	vec3 v = (vec3){c.x - a.x, c.y - a.y, c.z - a.z};
	return (vec3){u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};

}

float
    mod289
    ( float x )
{ return x - floorf(x * (1.0f / 289.0f)) * 289.0f; }

vec4
    perm
    ( vec4 x )

{

    vec4 p = vec4_mul(
        vec4_add(
            vec4_scale(x, 34.0f),
            (vec4){ 1.0f, 1.0f, 1.0f, 1.0f }
        ),
        x
    );

    p.x = mod289(p.x);
    p.y = mod289(p.y);
    p.z = mod289(p.z);
    p.w = mod289(p.w);
    return p;

}

vec3
    vec3_fract
    (vec3 x)

{ 

    vec3 p;
    p.x = x.x - floorf(x.x);
    p.y = x.y - floorf(x.y);
    p.z = x.z - floorf(x.z);
    
    return p; 

}

vec4
    vec4_fract
    ( vec4 x )

{ 

    vec4 p;
    p.x = x.x - floorf(x.x);
    p.y = x.y - floorf(x.y);
    p.z = x.z - floorf(x.z);
    p.w = x.w - floorf(x.w);
    
    return p; 

}



float
    fbm_noise
    ( vec3 p )
    
{
    
    vec3 a = (vec3){floorf(p.x), floorf(p.y), floorf(p.z)};
    vec3 d = vec3_sub(p, a);

    d = vec3_mul(vec3_mul(d, d), vec3_sub((vec3){3.0f, 3.0f, 3.0f}, vec3_scale(d, 2.0f)));

    vec4 b = vec4_add((vec4){a.x, a.x, a.y, a.y}, (vec4){0.0f, 1.0f, 0.0f, 1.0f});
    vec4 k1 = perm((vec4){b.x, b.y, b.x, b.y});
    vec4 k2 = perm(vec4_add((vec4){k1.x, k1.y, k1.x, k1.y}, (vec4){b.z, b.z, b.w, b.w}));

    vec4 c = vec4_add(k2, (vec4){a.z, a.z, a.z, a.z});
    vec4 k3 = perm(c);
    vec4 k4 = perm(vec4_add(c, (vec4){1.0f, 1.0f, 1.0f, 1.0f}));

    vec4 o1 = vec4_fract(vec4_scale(k3, (1.0f / 41.0f)));
    vec4 o2 = vec4_fract(vec4_scale(k4, (1.0f / 41.0f)));

    vec4 o3 = vec4_add(vec4_scale(o2, d.z), vec4_scale(o1, (1.0f - d.z)));
    vec2 o4 = vec2_add(vec2_scale((vec2){o3.y, o3.w}, d.x), vec2_scale((vec2){o3.x, o3.z}, (1.0f - d.x)));
    
    return o4.y * d.y + o4.x * (1.0 - d.y);

} 

float
    fbm
    ( vec3 p )
    
{
    
	float v = 0.0;
	float a = 0.5;
	float f = 1.0;
    
	vec3 shift = (vec3){100, 100, 100};
	for (int i = 0; i < 5; ++i) {

		v += a * fbm_noise(vec3_scale(p, f));
		
		p = vec3_add(vec3_scale(p, 2.0), shift);
		f *= 2.5;
		a *= 0.45;

	}
	
	return v;

}



float
    wrap
    (
        float x,
        float size
    )

{
    float r = fmodf(x, size);
    if (r < 0) r += size;
    return r;
}

vec3 vec3_wrap(vec3 a, float size)
{
    a.x = wrap(a.x, size);
    a.y = wrap(a.y, size);
    a.z = wrap(a.z, size);
    return a;
}


/**
 * @brief Samples a precomputed perlin volume.
 * @param p The position to sample.
 * @ingroup mesh_group
 */
float
    sample_density
    ( vec3 p )

{

    ivec3 pf = {floorf(p.x), floorf(p.y), floorf(p.z)};
    int pm1 = PERLIN_TEXTURE_SIZE - 1;

    pf.x &= pm1;
    pf.y &= pm1;
    pf.z &= pm1;
    int px1 = (int)(pf.x + 1) & pm1;
    int py1 = (int)(pf.y + 1) & pm1;
    int pz1 = (int)(pf.z + 1) & pm1;
    ivec3 pf1 = {px1, py1, pz1};


    float fx = p.x - floorf(p.x);
    float fy = p.y - floorf(p.y);
    float fz = p.z - floorf(p.z);


    float e0 = perlin_tex[ pf.z  * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf.y  * PERLIN_TEXTURE_SIZE + pf.x ];
    float e1 = perlin_tex[ pf.z  * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf.y  * PERLIN_TEXTURE_SIZE + pf1.x ];
    float e2 = perlin_tex[ pf.z  * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf1.y * PERLIN_TEXTURE_SIZE + pf.x ];
    float e3 = perlin_tex[ pf.z  * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf1.y * PERLIN_TEXTURE_SIZE + pf1.x ];
    float e4 = perlin_tex[ pf1.z * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf.y  * PERLIN_TEXTURE_SIZE + pf.x ];
    float e5 = perlin_tex[ pf1.z * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf.y  * PERLIN_TEXTURE_SIZE + pf1.x ];
    float e6 = perlin_tex[ pf1.z * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf1.y * PERLIN_TEXTURE_SIZE + pf.x ];
    float e7 = perlin_tex[ pf1.z * PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE + pf1.y * PERLIN_TEXTURE_SIZE + pf1.x ];

    float c00 = lerp(e0, e1, fx);
    float c10 = lerp(e2, e3, fx);
    float c01 = lerp(e4, e5, fx);
    float c11 = lerp(e6, e7, fx);

    float c0 = lerp(c00, c10, fy);
    float c1 = lerp(c01, c11, fy);

    float out = lerp(c0, c1, fz);

    return out * 2.0f - 1.0f;
    
}


/**
 * @brief Samples a precomputed fbm volume.
 * @param p The position to sample.
 * @ingroup mesh_group
 */
float
    sample_fbm_density
    ( vec3 p )

{

    ivec3 pf = {floorf(p.x), floorf(p.y), floorf(p.z)};
    

    pf.x &= FBM_TEXTURE_SIZE-1;
    pf.y &= FBM_TEXTURE_SIZE-1;
    pf.z &= FBM_TEXTURE_SIZE-1;
    int SIZE = FBM_TEXTURE_SIZE;
    int px1 = (int)(pf.x + 1) & (FBM_TEXTURE_SIZE-1);
    int py1 = (int)(pf.y + 1) & (FBM_TEXTURE_SIZE-1);
    int pz1 = (int)(pf.z + 1) & (FBM_TEXTURE_SIZE-1);
    ivec3 pf1 = {px1, py1, pz1};


    float fx = p.x - floorf(p.x);
    float fy = p.y - floorf(p.y);
    float fz = p.z - floorf(p.z);


    float e0 = fbm_tex[ pf.z  * SIZE * SIZE + pf.y  * SIZE + pf.x ];
    float e1 = fbm_tex[ pf.z  * SIZE * SIZE + pf.y  * SIZE + pf1.x ];
    float e2 = fbm_tex[ pf.z  * SIZE * SIZE + pf1.y * SIZE + pf.x ];
    float e3 = fbm_tex[ pf.z  * SIZE * SIZE + pf1.y * SIZE + pf1.x ];
    float e4 = fbm_tex[ pf1.z * SIZE * SIZE + pf.y  * SIZE + pf.x ];
    float e5 = fbm_tex[ pf1.z * SIZE * SIZE + pf.y  * SIZE + pf1.x ];
    float e6 = fbm_tex[ pf1.z * SIZE * SIZE + pf1.y * SIZE + pf.x ];
    float e7 = fbm_tex[ pf1.z * SIZE * SIZE + pf1.y * SIZE + pf1.x ];

    float c00 = lerp(e0, e1, fx);
    float c10 = lerp(e2, e3, fx);
    float c01 = lerp(e4, e5, fx);
    float c11 = lerp(e6, e7, fx);

    float c0 = lerp(c00, c10, fy);
    float c1 = lerp(c01, c11, fy);

    float out = lerp(c0, c1, fz);

    return out * 2.0f - 1.0f;
    
}


/**
 * @brief Copies the exact density function from the GPU to the CPU. Used to calculate collision.
 * @param p The position to calculate density for.
 * @return The density at position.
 * @ingroup mesh_group
 */
float
    calc_density
    ( vec3 p )
    
{

    float d = p.z;

    vec3 chunk_origin = vec3_scale(p, (float) 1 / CHUNK_WORLD_SIZE);
    chunk_origin.x = floorf(chunk_origin.x);
    chunk_origin.y = floorf(chunk_origin.y);
    chunk_origin.z = floorf(chunk_origin.z);
    chunk_origin = vec3_scale(chunk_origin, CHUNK_WORLD_SIZE);

    
    // Apply equal density & noise calculations to density.comp
    // UV has to be adjusted to wrap the noise textures and match opengl wrapping
    for (int i = 0; i < PLANET_DATA.warp_count; i++)
    {
        
        WarpLayer * wl = &PLANET_DATA.warp_layers[i];
        vec3 uvw = vec3_add(vec3_scale(p, wl->freq0), wl->offset0);
        uvw.x -= floorf(uvw.x);
        uvw.y -= floorf(uvw.y);
        uvw.z -= floorf(uvw.z);
        vec3 w1 = vec3_add(vec3_scale(uvw, PERLIN_TEXTURE_SIZE), (vec3){-0.5f, -0.5f, -0.5f});

        uvw = vec3_add(vec3_scale(p, wl->freq1), wl->offset1);
        uvw.x -= floorf(uvw.x);
        uvw.y -= floorf(uvw.y);
        uvw.z -= floorf(uvw.z);
        vec3 w2 = vec3_add(vec3_scale(uvw, PERLIN_TEXTURE_SIZE), (vec3){-0.5f, -0.5f, -0.5f});

        uvw = vec3_add(vec3_scale(p, wl->freq2), wl->offset2);
        uvw.x -= floorf(uvw.x);
        uvw.y -= floorf(uvw.y);
        uvw.z -= floorf(uvw.z);
        vec3 w3 = vec3_add(vec3_scale(uvw, PERLIN_TEXTURE_SIZE), (vec3){-0.5f, -0.5f, -0.5f});

        vec3 warp = {sample_density(w1), sample_density(w2), sample_density(w3)};
        p = vec3_add(p, vec3_scale(warp, wl->amp));
        
    }
    
    for (int i = 0; i < PLANET_DATA.perlin_count; i++)
    {
        
        PerlinLayer * pl = &PLANET_DATA.perlin_layers[i];
        vec3 uvw = vec3_scale(p, pl->freq);
        uvw.x -= floorf(uvw.x);
        uvw.y -= floorf(uvw.y);
        uvw.z -= floorf(uvw.z);

        vec3 t = vec3_add(vec3_scale(uvw, PERLIN_TEXTURE_SIZE), (vec3){-0.5f, -0.5f, -0.5f});
        float d0 = sample_density(t) * pl->amp;
        d += d0;

    }

    for (int i = 0; i < PLANET_DATA.fbm_count; i++)
    {

        FBMLayer * fl = &PLANET_DATA.fbm_layers[i];
        vec3 uvw = vec3_scale(p, fl->freq);
        uvw.x -= floorf(uvw.x);
        uvw.y -= floorf(uvw.y);
        uvw.z -= floorf(uvw.z);
        vec3 t = vec3_add(vec3_scale(uvw, FBM_TEXTURE_SIZE), (vec3){-0.5f, -0.5f, -0.5f});
        float d0 = sample_fbm_density(t) * fl->amp;
        d += d0;

    }

    return d;

}

// for each voxel 0.. 33^3
// scale by voxel size and add to chunk offset
// calculate density
// slot density into 33^3 grid
//
// marching cubes per 0.. 32
// append to vertex buffer
// atomically increment count by 3
// append tri verts
//
// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
//                 GL_ATOMIC_COUNTER_BARRIER_BIT);
//
// glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
// draw


// skirts
// 
// vec3 v0 = original_vertex;
// vec3 v1 = v0 - normal * skirt_depth;
// (v0, v0_next, v1)
// (v1, v0_next, v1_next)

// Create a mesh for a certain chunk
/**
 * @brief Creates a mesh for a certain chunk.
 * @param lod The LOD level to generate.
 * @param chunk_center The chunk center in world space.
 * @param chunk_id The ID of the chunk in ::CHUNK_DATA.
 * @param is_parent If the chunk is that of higher LOD than child. This makes it so that the parent always reports the vertex count.
 * @ingroup mesh_group
 */
void
    create_new_planet_mesh
    (
        int      lod,
        ivec3    chunk_center,
        int      chunk_id,
        bool     is_parent
    )

{

    int size = LOD_RES[lod];
    uint32_t uid = CHUNK_DATA[lod][chunk_id].uID;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, DENSITY_SSBO[lod]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, DENSITY_VERTEX_SSBO[lod]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, INDIRECT_BUFFER[lod][chunk_id]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, PERLIN_TEXTURE);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, FBM_TEXTURE);

    // Density function computes the SDF
    glUseProgram(DENSITY_COMPUTE_SHADER.program);
    glUniform1i(DENSITY_COMPUTE_SHADER.uniforms[0].location, size);
    glUniform1i(DENSITY_COMPUTE_SHADER.uniforms[1].location, chunk_id);
    glUniform3f(DENSITY_COMPUTE_SHADER.uniforms[2].location, chunk_center.x, chunk_center.y, chunk_center.z);
    glUniform1i(DENSITY_COMPUTE_SHADER.uniforms[3].location, lod);
    glUniform1i(DENSITY_COMPUTE_SHADER.uniforms[4].location, 0);
    glUniform1i(DENSITY_COMPUTE_SHADER.uniforms[5].location, 1);
    glDispatchCompute(size+1, size+1, size+1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Marching cubes the mesh
    glUseProgram(MARCHING_CUBES_COMPUTE_SHADER.program);
    glUniform1i(MARCHING_CUBES_COMPUTE_SHADER.uniforms[0].location, size);
    glUniform1i(MARCHING_CUBES_COMPUTE_SHADER.uniforms[1].location, chunk_id);
    glUniform3f(MARCHING_CUBES_COMPUTE_SHADER.uniforms[2].location, chunk_center.x, chunk_center.y, chunk_center.z);
    glUniform1i(MARCHING_CUBES_COMPUTE_SHADER.uniforms[3].location, 0);
    glUniform1i(MARCHING_CUBES_COMPUTE_SHADER.uniforms[4].location, 1);
    glDispatchCompute(size, size, size);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    // Generates an indirect draw command
    glUseProgram(INDIRECT_COMPUTE_SHADER.program);
    glUniform1i(INDIRECT_COMPUTE_SHADER.uniforms[0].location, chunk_id);
    glUniform1i(INDIRECT_COMPUTE_SHADER.uniforms[1].location, size);
    glUniform1i(INDIRECT_COMPUTE_SHADER.uniforms[4].location, lod);
    glUniform1i(glGetUniformLocation(INDIRECT_COMPUTE_SHADER.program, "is_parent"), is_parent);
    glUniform1i(glGetUniformLocation(INDIRECT_COMPUTE_SHADER.program, "chunk_uid"), uid);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    
        
}

bool
    aabb_in_frustum
    (
        ivec3 min,
        ivec3 max,
        vec4 planes[6]
    )

{

    for (int i = 0; i < 6; i++)
    {
        vec3 p = {
            planes[i].x > 0 ? max.x : min.x,
            planes[i].y > 0 ? max.y : min.y,
            planes[i].z > 0 ? max.z : min.z
        };

        if (vec3_dot((vec3){planes[i].x, planes[i].y, planes[i].z}, p) + planes[i].w < 0)
        {
            return false;
        }
    }

    return true;

}


// Struct for the scores that determines how valuable a chunk is to render


/**
 * @brief Returns view planes from a camera, used for frustum culling.
 */
void
    get_view_planes
    (
        vec4     planes[6],
        Camera * cam,
        vec3     planet_offset,
        float    planet_radius
    )

{

    vec3 cam_pos = (vec3){
        cam->cam_data.cam_pos.x - planet_offset.x,
        cam->cam_data.cam_pos.y - planet_offset.y,
        cam->cam_data.cam_pos.z - planet_offset.z - planet_radius
    };

    Camera fake_cam;
    fake_cam.cam_data.cam_pos = (vec4){cam_pos.x, cam_pos.y, cam_pos.z, 0};
    fake_cam.pitch = cam->pitch;
    fake_cam.yaw = cam->yaw;
    
    update_camera(&fake_cam);

    mat4 VP = m4_mul(fake_cam.cam_data.proj, fake_cam.cam_data.view);

    vec4 r0 = (vec4){VP.a0, VP.b0, VP.c0, VP.d0};
    vec4 r1 = (vec4){VP.a1, VP.b1, VP.c1, VP.d1};
    vec4 r2 = (vec4){VP.a2, VP.b2, VP.c2, VP.d2};
    vec4 r3 = (vec4){VP.a3, VP.b3, VP.c3, VP.d3};

    planes[0] = vec4_add(r3, r0);
    planes[1] = vec4_sub(r3, r0);
    planes[2] = vec4_add(r3, r1);
    planes[3] = vec4_sub(r3, r1);
    planes[4] = vec4_add(r3, r2);
    planes[5] = vec4_sub(r3, r2);
    
}


// qsort functions for sorting chunk jobs
int
    compareJobs
    (
        const void * a,
        const void * b
    )

{

    ChunkScore * a0 = (ChunkScore*)a;
    ChunkScore * b0 = (ChunkScore*)b;
    
    if (b0->score > a0->score) return 1;
    if (b0->score < a0->score) return -1;
    return 0;

}

int
    compareFrees
    (
        const void * a,
        const void * b
    )

{

    ChunkScore * a0 = (ChunkScore*)a;
    ChunkScore * b0 = (ChunkScore*)b;
    
    if (b0->score < a0->score) return 1;
    if (b0->score > a0->score) return -1;
    return 0;

}



// this is how we do LODS:
// calculate density 
// calculate MC at low res
// present
// LOD increase
// calculate density
// calculate MC at high res
// store separately
// interpolate based on distance


AuthorityEntry* authority_map = NULL;
ChunkHash * hash_table = NULL;

// The chunks that have already been emitted this frame
// Used so that we don't touch chunks that we just made.
// Copied over on each new frame so that we can have a list of chunks we made during the last frame. Used for the zero list.
bool already_emitted[LODMAX][MAX_CHUNKS];

/**
 * @brief Finds the highest LOD that we have cached from the current chunk.
 * @ingroup mesh_group
 */
int
    find_current_lod
    (
        ivec3        center,
        ChunkHash ** h
    )

{
    
    ChunkHash* best = NULL;
    int best_lod = -1;

    for (int l = 0; l < LODMAX; l++) {

        ChunkHash* h = find_chunk_hash(center, l);
        if (h && l > best_lod) {
            best = h;
            best_lod = l;
        }

    }

    *h = best;
    return best_lod;

}

/**
 * @brief Frees the given LOD (as a parent).
 * @ingroup mesh_group
 */
bool
    free_current_parent
    (
        int lod,
        int i
    )

{

    if (lod < 0 || lod >= LODMAX) return false;

    if (!CHUNK_DATA[lod][i].free)
    {

        delete_chunk_hash(CHUNK_DATA[lod][i].center, lod);
        CHUNK_DATA[lod][i].free = true;
        CHUNK_DATA[lod][i].is_child = false;
        return true;

    }
    
    return false;

}

/**
 * @brief Frees the parent of given LOD.
 * @ingroup mesh_group
 */
bool
    free_parent
    (
        int lod,
        int i
    )

{

    if (lod < 0 || lod >= LODMAX) return false;
    int i1 = CHUNK_DATA[lod][i].id1;

    if (CHUNK_DATA[lod][i].id1 != -1 && lod - 1 >= 0)
    {

        if (CHUNK_DATA[lod-1][i1].id0 == i && !already_emitted[lod-1][i1] && !CHUNK_DATA[lod-1][i1].free)
        {
            
            delete_chunk_hash(CHUNK_DATA[lod-1][i1].center, lod-1);
            CHUNK_DATA[lod-1][i1].free = true;
            CHUNK_DATA[lod-1][i1].is_child = false;
            return true;

        }

    }
    return false;

}

/**
 * @brief Frees the child of given LOD.
 * @ingroup mesh_group
 */
bool
    free_child
    (
        int lod,
        int i
    )

{

    if (lod < 0 || lod >= LODMAX) return false;
    int i1 = CHUNK_DATA[lod][i].id0;

    if (CHUNK_DATA[lod][i].id0 != -1 && lod + 1 < LODMAX)
    {

        if (CHUNK_DATA[lod+1][i1].id1 == i && !already_emitted[lod+1][i1] && !CHUNK_DATA[lod+1][i1].free)
        {

            delete_chunk_hash(CHUNK_DATA[lod+1][i1].center, lod+1);
            CHUNK_DATA[lod+1][i1].free = true;
            CHUNK_DATA[lod+1][i1].is_child = false;
            return true;

        }
        
    }
    return false;
    
}


uint32_t chunk_uID = 0;

uint32_t
    assign_new_uID()
{ chunk_uID = (chunk_uID + 1) % UINT32_MAX; return chunk_uID; }


// The zero cache is a hash table of known empty chunks so that we don't keep generating them at higher LODs and skip over them.
// AKA below ground or above ground chunks with nothing inside of them.
#define MAX_ZERO_LIST 100000

typedef struct ZeroKey
{
    int x, y, z, lod;

} ZeroKey;

typedef struct ZeroEntry
{
    ZeroKey key;
    UT_hash_handle hh;

} ZeroEntry;

ZeroEntry * zero_hash_table = NULL;
int zero_list_count = 0;


ChunkScore * jobs = NULL;
ChunkData * job_data = NULL;
ChunkScore * frees = NULL;
// Mask of chunks that are going to be replaced during the frame
bool replacement_mask[LODMAX][MAX_CHUNKS] = {0};



// Inits the zero list to -1 (empty value)
/**
 * @brief Initializes and frees old ChunkData and hash tables.
 * @ingroup mesh_group
 */
void
    init_chunk_data ()

{
    {
        ZeroEntry * s;
        ZeroEntry * tmp;
        HASH_ITER(hh, zero_hash_table, s, tmp){
            HASH_DEL(zero_hash_table, s);
            free(s);

        }
    }
    {
        
        ChunkHash * s;
        ChunkHash * tmp;
        HASH_ITER(hh, hash_table, s, tmp){
            HASH_DEL(hash_table, s);
            free(s);

        }
    }
    {
        
        AuthorityEntry * s;
        AuthorityEntry * tmp;
        HASH_ITER(hh, authority_map, s, tmp){
            HASH_DEL(authority_map, s);
            free(s);

        }
    }
    if (jobs)
    {
        free(jobs);
        jobs = NULL;
    }
    if (job_data)
    {
        free(job_data);
        job_data = NULL;
    }
    if (frees)
    {
        free(frees);
        frees = NULL;
    }

    jobs = calloc(MAX_CHUNK_LIST * LODMAX, sizeof(ChunkScore));
    job_data = calloc(MAX_CHUNK_LIST * LODMAX, sizeof(ChunkData));
    frees = calloc(MAX_CHUNKS, sizeof(ChunkScore));
    memset(CHUNK_DATA, 0, sizeof(ChunkData) * LODMAX * MAX_CHUNKS);
    for (int l = 0; l < LODMAX; l++)
    {
        for (int i = 0; i < MAX_CHUNKS_LOD[l]; i++)
        {
            glGenBuffers(1, &INDIRECT_BUFFER[l][i]);

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, INDIRECT_BUFFER[l][i]);
            glBufferData(GL_DRAW_INDIRECT_BUFFER,
                         sizeof(DrawArraysIndirectCommand),
                         NULL,
                         GL_DYNAMIC_DRAW);
        }
        
    }
    for (int l = 0; l < LODMAX; l++)
    {
        for (int i = 0; i < MAX_CHUNKS; i++)
        {
            CHUNK_DATA[l][i].free = true;
            CHUNK_DATA[l][i].is_child = false;
            CHUNK_DATA[l][i].uID = -1;
            CHUNK_DATA[l][i].id0 = -1;
            CHUNK_DATA[l][i].id1 = -1;
        
        }
    }

}

int generation_id = 0;

int emitted_chunks = 0;
int emitted_chunk_ids[VERTEX_COUNT_MAX];
int emitted_chunk_lods[VERTEX_COUNT_MAX];


GLsync zero_fence; 

/**
 * @brief Checks all chunks based on new Camera position, building a job list and then executing them based on ChunkScore.
 * @param planet_origin The origin to determine the relative noise value.
 * @param planet_radius The radius to determine the height offset needed to find the planet surface from the planet center.
 * @ingroup mesh_group
 */
void
    load_chunks
    (
        vec3   planet_origin,
        float  planet_radius
    )

{

    generation_id++;

    Timer start, big_loop, execute;

    start_timer(&start);

    // Copies old chunk emissions into a back buffer
    bool old_emissions[LODMAX][MAX_CHUNKS];
    memcpy(old_emissions, already_emitted, sizeof(already_emitted));
    memset(already_emitted, 0, sizeof(already_emitted));
    memset(replacement_mask, 0, sizeof(replacement_mask));

    // Swaps the chunk data to the back buffer
    int read_flag = CHUNK_DATA_FLAG;

    GLenum res = glClientWaitSync(zero_fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);

    if (res == GL_CONDITION_SATISFIED || res == GL_ALREADY_SIGNALED || emitted_chunks >= VERTEX_COUNT_MAX - LODMAX * MAX_CHUNKS)
    {
        CHUNK_DATA_FLAG = (CHUNK_DATA_FLAG + 1) % 3;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, CHUNK_DATA_SSBO[CHUNK_DATA_FLAG]);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, CHUNK_DATA_SSBO[read_flag]);
        // Maps the last frame's chunk data vertex counts
        VertexInfo * ptr = glMapBufferRange(
            GL_SHADER_STORAGE_BUFFER,
            offsetof(GPUChunk, vertex_counts),
            sizeof(VertexInfo) * emitted_chunks,
            GL_MAP_READ_BIT
        );

        VertexInfo new_list[VERTEX_COUNT_MAX];
        memcpy(new_list, ptr, sizeof(VertexInfo) * emitted_chunks);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        // Zeroes the back buffer
        glUseProgram(CHUNK_ZERO_COMPUTE_SHADER); 
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // If an emitted chunk has 0 vertices, free it and add it to the zero hash table.
        for (int c = 0; c < emitted_chunks; c++)
        {

            int l = emitted_chunk_lods[c];
            int i = emitted_chunk_ids[c];
    
            if (new_list[c].vcnt == 0 && new_list[c].uid == CHUNK_DATA[l][i].uID)
            {

                free_child(l, i);
                free_current_parent(l, i);

                ZeroKey k = {CHUNK_DATA[l][i].center.x, CHUNK_DATA[l][i].center.y, CHUNK_DATA[l][i].center.z, l};
                ZeroEntry * m = NULL;
                HASH_FIND(hh, zero_hash_table, &k, sizeof(ZeroKey), m);
                if (!m)
                {
            
                    ZeroEntry * z = malloc(sizeof(ZeroEntry));
                    memset(z, 0, sizeof(ZeroEntry));
                    z->key = k;
                    HASH_ADD(hh, zero_hash_table, key, sizeof(ZeroKey), z); 
           
                }

        
                zero_list_count++;
            }

        }
        emitted_chunks = 0;
    }

    
    const float lookahead = 0.25f;
    // have to move cam pos into planet coords and applies lookahead based on camera velocity
    ivec3 cam_pos = (ivec3){
        current_cam->cam_data.cam_pos.x + current_cam->cam_data.cam_vel.x * lookahead,
        current_cam->cam_data.cam_pos.y + current_cam->cam_data.cam_vel.y * lookahead,
        current_cam->cam_data.cam_pos.z + current_cam->cam_data.cam_vel.z * lookahead
    };
    cam_pos.x -= planet_origin.x;
    cam_pos.y -= planet_origin.y;
    cam_pos.z -= planet_origin.z;
    cam_pos.z -= planet_radius;


    if (zero_list_count >= MAX_ZERO_LIST)
    {
        zero_list_count = 0;

        ZeroEntry * s;
        ZeroEntry * tmp;
        HASH_ITER(hh, zero_hash_table, s, tmp){
            ivec3 center = (ivec3){s->key.x, s->key.y, s->key.z};
            float dist = ivec3_dist(center, cam_pos);
            if (dist > CHUNK_RADIUS * CHUNK_WORLD_SIZE)
            {
                HASH_DEL(zero_hash_table, s);
                free(s);
            }

        }
    }

    // Set up view planes for frustum culling.
    vec4 planes[6];
    vec4 light_planes[6];
    get_view_planes(planes, current_cam, planet_origin, planet_radius);
    get_view_planes(light_planes, &LIGHT_CAM, planet_origin, planet_radius);
    

    // This particle struct was supposed to change the terrain based on material makeup.

   
    int free_count = 0;
    float frustum_weight = 100;
    int cam_chunk = -1;
    int cam_chunk_lod = 0;

    ivec3 MIN_OFFSET = {
        -CHUNK_WORLD_SIZE / 2,
        -CHUNK_WORLD_SIZE / 2,
        -CHUNK_WORLD_SIZE / 2
    };

    ivec3 MAX_OFFSET = {
        CHUNK_WORLD_SIZE / 2,
        CHUNK_WORLD_SIZE / 2,
        CHUNK_WORLD_SIZE / 2
    };


    vec3 pos = ivec3_scale(cam_pos, 1.0f/CHUNK_WORLD_SIZE);
    pos.x = floorf(pos.x);
    pos.y = floorf(pos.y);
    pos.z = floorf(pos.z);
    pos = vec3_scale(pos, CHUNK_WORLD_SIZE);

    // Check for if we have a chunk that houses the camera.
    ivec3 cam_center_chunk = {pos.x, pos.y, pos.z}; 
    int cam_clod = get_current_lod(cam_center_chunk);
    ChunkHash * h = NULL;
    h = find_chunk_hash(cam_center_chunk, cam_clod);
    if (h)
    {
        
        cam_chunk = h->id;
        cam_chunk_lod = cam_clod;
    }

    // Start is the bottom left chunk in the cube that we are iterating through. Center is the center.
    ivec3 start_pos;
    ivec3 center_pos;

    // If our camera isn't found within a chunk we have generated, we start at the bottom left voxel in the cube and center based on our camera position.
    if (cam_chunk == -1 /* || closest_dist > CHUNK_WORLD_SIZE * CHUNK_RADIUS */)
    {

        start_pos = cam_pos;
        center_pos = cam_center_chunk;
        start_pos = ivec3_add(center_pos, (ivec3){-CHUNK_WORLD_SIZE * CHUNK_RADIUS, -CHUNK_WORLD_SIZE * CHUNK_RADIUS, -CHUNK_WORLD_SIZE * CHUNK_RADIUS});

    }
    else
    {
        
        start_pos = ivec3_add(
            CHUNK_DATA[cam_chunk_lod][cam_chunk].center,
            (ivec3){-CHUNK_WORLD_SIZE * CHUNK_RADIUS, -CHUNK_WORLD_SIZE * CHUNK_RADIUS, -CHUNK_WORLD_SIZE * CHUNK_RADIUS}
        );
        center_pos = CHUNK_DATA[cam_chunk_lod][cam_chunk].center;
        
    }


    double start_time = end_timer(&start);

    start_timer(&big_loop);
    // The current chunk position
    ivec3 chunk_pos = start_pos;

    int job_count = 0;
    
    // Iterate through the cube volume and push potential chunks that need to be updated in LOD or brand new chunks.
    for (int z = 0; z < CHUNK_RADIUS * 2 + 1; z++)
    {

        for (int y = 0; y < CHUNK_RADIUS * 2 + 1; y++)
        {

            for (int x = 0; x < CHUNK_RADIUS * 2 + 1; x++)
            {

                ivec3 min = ivec3_add(chunk_pos, MIN_OFFSET);
                ivec3 max = ivec3_add(chunk_pos, MAX_OFFSET);

                // There was an idea to not push chunks based on direction but not sure if it works or not.
                // ivec3 dir = ivec3_sub(cam_pos, chunk_pos);
                // vec3 ndir = ivec3_normalize(dir);
                // float dot = ndir.x*cam.forward.x + ndir.y*cam.forward.y + ndir.z*cam.forward.z;

                // if (dot > 0)
                // {
                //     chunk_pos = ivec3_add(start_pos, (ivec3){x * CHUNK_WORLD_SIZE, y * CHUNK_WORLD_SIZE, z * CHUNK_WORLD_SIZE});
                //     continue;
                // }
                
                // Don't push jobs outside of frustum.
                // bool within_frustum = aabb_in_frustum(min, max, planes) || aabb_in_frustum(min, max, light_planes);
                bool within_frustum = aabb_in_frustum(min, max, planes);
                if (!within_frustum)
                {
                    
                    chunk_pos = ivec3_add(start_pos, (ivec3){x * CHUNK_WORLD_SIZE, y * CHUNK_WORLD_SIZE, z * CHUNK_WORLD_SIZE});
                    continue;
                }

                // Determine LOD level based on distance
                int lod_level = 0;
                float dist_to_center = ivec3_dist(chunk_pos, cam_pos);

                if (dist_to_center < CHUNK_WORLD_SIZE * 2 - 1e-4) { lod_level = 0; }
                else if (dist_to_center < CHUNK_WORLD_SIZE * 4 - 1e-4) { lod_level = 1; }
                else if (dist_to_center < CHUNK_WORLD_SIZE * 6 - 1e-4) { lod_level = 2; }
                else { lod_level = 3; }
       
                ZeroEntry * ze = NULL;
                ZeroKey key = {chunk_pos.x, chunk_pos.y, chunk_pos.z, lod_level};
                HASH_FIND(hh, zero_hash_table, &key, sizeof(ZeroKey), ze);
                if (ze)
                {

                    chunk_pos = ivec3_add(
                        start_pos,
                        (ivec3){x * CHUNK_WORLD_SIZE, y * CHUNK_WORLD_SIZE, z * CHUNK_WORLD_SIZE}
                    );
                    continue;
                    
                }

                bool already_done = false;
                int updating_lod_level = -1;
                int updating_lod_index = -1;

                // Get current highest LOD cached for a chunk.
                int clod = get_current_lod(chunk_pos);
                ChunkHash * h = NULL;
                h = find_chunk_hash(chunk_pos, clod);

                // Process which LOD level we will propose.
                if (h)
                {
                    
                    if (clod == lod_level) { already_done = true; }
                    else
                    {
                        
                        updating_lod_level = clod;
                        updating_lod_index = h->id;
                        lod_level = MAX(MIN(lod_level, clod + 1), clod - 1);
                        if (lod_level == clod)
                        {
                            already_done = true;
                        }
                    }

                }
                

                // If chunk is already finished or we were going to process it at the same LOD, skip.
                if (already_done)
                {
                    chunk_pos = ivec3_add(start_pos, (ivec3){x * CHUNK_WORLD_SIZE, y * CHUNK_WORLD_SIZE, z * CHUNK_WORLD_SIZE});
                    continue;
                }
                
                // If we are updating, set replacement values.
                if (updating_lod_level != -1)
                {
                    job_data[job_count].replacement_lod = updating_lod_level;
                    job_data[job_count].replacement_index = updating_lod_index;
                    replacement_mask[updating_lod_level][updating_lod_index] = true;
                }
                else
                {
                    job_data[job_count].replacement_lod = -1;
                    job_data[job_count].replacement_index = -1;
                }

                
                // Job data is different than jobs because it is a bigger struct and qsort is smaller.
                job_data[job_count].min = min;
                job_data[job_count].max = max;
                job_data[job_count].center = chunk_pos;
                job_data[job_count].lod = lod_level;
                job_data[job_count].free = false;

                float weight = (1 - (dist_to_center / (float)(CHUNK_WORLD_SIZE * 20)) + 1) * (within_frustum * frustum_weight + 1);

                jobs[job_count].score = weight;
                jobs[job_count].index = job_count;
                jobs[job_count].lod = lod_level;
                jobs[job_count].active = true;

                job_count++;

                chunk_pos = ivec3_add(start_pos, (ivec3){x * CHUNK_WORLD_SIZE, y * CHUNK_WORLD_SIZE, z * CHUNK_WORLD_SIZE});

            }
        }
    }

    // Score all current chunks based on importance and skip over children or empty chunks.
    float min_score = FLT_MAX;
    for (int l = 0; l < LODMAX; l++)
    {
        
        for (int i = 0; i < MAX_CHUNKS_LOD[l]; i++)
        {

            if (CHUNK_DATA[l][i].is_child && !CHUNK_DATA[l][i].free){
                
                CHUNK_DATA[l][i].within_frustum = false;
                continue;  

            } 
            
            if (CHUNK_DATA[l][i].free)
            {

                frees[free_count].index = i;
                frees[free_count].lod = l;
                frees[free_count].active = true;
                frees[free_count].score = -99;
                CHUNK_DATA[l][i].within_frustum = false;
                free_count++;
                min_score = -99;
                continue;

            }
            else
            {

                if (replacement_mask[l][i]) continue;
                frees[free_count].index = i;
                frees[free_count].lod = l;
                frees[free_count].active = true;

                ivec3 min = CHUNK_DATA[l][i].min;
                ivec3 max = CHUNK_DATA[l][i].max;

                bool within_frustum = aabb_in_frustum(min, max, planes);
                CHUNK_DATA[l][i].within_frustum = within_frustum;

                float dist = ivec3_dist(CHUNK_DATA[l][i].center, cam_pos);

                float weight = (1 - (dist / (float)(CHUNK_WORLD_SIZE * 20)) + 1) * (within_frustum * frustum_weight + 1);

                frees[free_count].score = weight;
                if (weight < min_score) { min_score = weight; }

                free_count++;
            }
        }
    }

    // Sort both jobs and current free chunks.
    qsort(frees, free_count, sizeof(ChunkScore), compareFrees);
    qsort(jobs, job_count, sizeof(ChunkScore), compareJobs);

    double big_loop_time = end_timer(&big_loop);

    start_timer(&execute);


    // Execute jobs that have higher importance over least important free chunks.
    for (int i = 0; i < job_count; i++)
    {

        int j = jobs[i].index;
        if (!jobs[i].active || jobs[i].score <= min_score) continue;
        int lod = job_data[j].lod;

        ivec3 center = job_data[j].center;

        if (job_data[j].replacement_index != -1)
        {

            if (already_emitted[job_data[j].replacement_lod][job_data[j].replacement_index]) continue;
            if (lod > job_data[j].replacement_lod)
            {
                lod += 1;
            }

        }
            
        int f = -1;
        bool out_of_space = false;
        bool skip_free = false;
        if (lod >= LODMAX) skip_free = true;
        int cf = -1;
        int k0 = -1;
        int k1 = -1;
        bool valid_child = false;
        while (f == -1 && !skip_free)
        {
            
            bool found_k = false;
            for (int k = 0; k < free_count; k++)
            {
                
                if
                (
                    frees[k].active &&
                    frees[k].lod == lod &&
                    frees[k].score < jobs[i].score
                )
                {

                    if (!already_emitted[frees[k].lod][frees[k].index])
                    {

                        f = k;
                        found_k = true;
                        k0 = k;

                        int child = CHUNK_DATA[frees[k].lod][frees[k].index].id0;

                        if
                        (
                            child != -1 &&
                            frees[k].lod + 1 < LODMAX &&
                            !CHUNK_DATA[lod][frees[k].index].free &&
                            !already_emitted[frees[k].lod+1][child]
                        )
                        { valid_child = true; }
                        
                        break;
                    }
                }
            }

            if
            (
                job_data[j].replacement_index == -1 &&
                f != -1 &&
                lod + 1 < LODMAX && !valid_child
            )
            {
                
                found_k = false;
                for (int k = 0; k < free_count; k++)
                {
                    
                    if
                    (
                        frees[k].active &&
                        frees[k].lod == lod + 1 &&
                        frees[k].score == -99
                    )
                    {

                        if (!already_emitted[frees[k].lod][frees[k].index]){
                        
                            found_k = true;
                            cf = frees[k].index;
                            frees[k].active = false;
                            k1 = k;
                            break;

                        }
                    }
                }
            }
            
            if (found_k)
            {

                frees[f].active = false;
                break;
            } else { f = -1; }
           
            if (f == -1){

                // special case: replacement with no space
                if (job_data[j].replacement_index != -1)
                {

                    out_of_space = true;
                    break;

                }
                lod++;
                if (lod >= LODMAX)
                {
                    out_of_space = true;
                    break;
                }
            }
            else
            {
                break;
            }
            
        }

        if (out_of_space) continue;
        if
        (
            job_data[j].replacement_index == -1 &&
            cf == -1 &&
            lod < LODMAX - 1 &&
            !valid_child
        )
            continue;

        int k = (f != -1) ? frees[f].index : -1;

        if (valid_child) { cf = CHUNK_DATA[lod][k].id0; }

        // Check the zero list to make sure we don't emit a duplicate empty chunk.
        bool match_found = false;
        if (job_data[j].replacement_index == -1)
        {
            
            ZeroEntry * ze = NULL;
            ZeroKey key =
            {
                job_data[j].center.x,
                job_data[j].center.y,
                job_data[j].center.z,
                lod
            };

            HASH_FIND(hh, zero_hash_table, &key, sizeof(ZeroKey), ze);
            if (ze)
            {
                
                if (k0 != -1) frees[k0].active = true;
                if (k1 != -1) frees[k1].active = true;
                continue;
                
            }
            
        }
       
        bool new_chunk = false;
        bool is_parent = false;


        // If we are replacing a chunk.
        if (job_data[j].replacement_index != -1)
        {

            // If going down in LOD.
            if (lod < job_data[j].replacement_lod)
            {

                assert(lod == job_data[j].replacement_lod - 1);
                assert(k != -1);
                uint32_t uID = CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].uID;
                free_child(lod, k);
                free_parent(lod, k);
                free_current_parent(lod, k);
                assert(!already_emitted[lod][k]);
                free_child(job_data[j].replacement_lod, job_data[j].replacement_index);
                assert(!already_emitted[job_data[j].replacement_lod][job_data[j].replacement_index]);
                    
                CHUNK_DATA[lod][k].uID = uID;
                CHUNK_DATA[lod][k].center = job_data[j].center;
                CHUNK_DATA[lod][k].min = job_data[j].min;
                CHUNK_DATA[lod][k].max = job_data[j].max;
                CHUNK_DATA[lod][k].free = false;
                already_emitted[lod][k] = true;
                
                CHUNK_DATA[lod][k].id0 = job_data[j].replacement_index;
                CHUNK_DATA[lod][k].id1 = -1;
                CHUNK_DATA[lod][k].is_child = false;
                is_parent = true;
                already_emitted[job_data[j].replacement_lod][job_data[j].replacement_index] = true;

                CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].center = job_data[j].center;
                CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].min = job_data[j].min;
                CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].max = job_data[j].max;
                CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].id0 = -1;
                CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].id1 = k;
                CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].is_child = true;
                CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].free = false;

                replace_chunk_hash(job_data[j].center, job_data[j].replacement_lod, job_data[j].replacement_index);
                replace_chunk_hash(job_data[j].center, lod, k);
                set_current_lod(job_data[j].center, lod);

            }
            // If going up in LOD.
            else
            {

                uint32_t uID = CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].uID;
                int child = CHUNK_DATA[job_data[j].replacement_lod][job_data[j].replacement_index].id0;

                assert(lod != job_data[j].replacement_lod);
                assert(lod == job_data[j].replacement_lod + 2);
                assert(child != -1);
                free_current_parent(job_data[j].replacement_lod, job_data[j].replacement_index);
                assert(!already_emitted[job_data[j].replacement_lod+1][child]);

                if (k != -1)
                {
                    
                    free_child(lod, k);
                    free_parent(lod, k);
                    free_current_parent(lod, k);
                    assert(!already_emitted[lod][k]);

                }


                assert(k != -1 || lod >= LODMAX);
                
                already_emitted[job_data[j].replacement_lod+1][child] = true;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].id0 = -1;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].id1 = -1;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].uID = uID;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].center = job_data[j].center;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].min = job_data[j].min;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].max = job_data[j].max;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].is_child = false;
                CHUNK_DATA[job_data[j].replacement_lod+1][child].free = false;

                if (k != -1)
                {

                    assert(lod != job_data[j].replacement_lod);
                    CHUNK_DATA[lod][k].uID = uID;
                    CHUNK_DATA[lod][k].id0 = -1;
                    CHUNK_DATA[lod][k].id1 = child;
                    CHUNK_DATA[lod][k].is_child = true;
                    CHUNK_DATA[lod][k].center = job_data[j].center;
                    CHUNK_DATA[lod][k].min = job_data[j].min;
                    CHUNK_DATA[lod][k].max = job_data[j].max;
                    CHUNK_DATA[lod][k].free = false;
                    already_emitted[lod][k] = true;
                    CHUNK_DATA[job_data[j].replacement_lod+1][child].id0 = k;
                    replace_chunk_hash(job_data[j].center, lod, k);

                }

                replace_chunk_hash(job_data[j].center, job_data[j].replacement_lod + 1, child);
                set_current_lod(job_data[j].center, job_data[j].replacement_lod + 1);

            }

        }
        // Brand new chunk.
        else
        {

            uint32_t uid = assign_new_uID();

            assert(k != -1);
            free_child(lod, k);
            free_parent(lod, k);
            free_current_parent(lod, k);
            assert(!already_emitted[lod][k]);

            if (lod + 1 < LODMAX)
            {

                free_child(lod+1, cf);
                free_parent(lod+1, cf);
                free_current_parent(lod+1, cf);
                assert(!already_emitted[lod+1][cf]);

            }

            int current_chunk = uid;
            CHUNK_DATA[lod][k].uID = current_chunk;
            CHUNK_DATA[lod][k].is_child = false;
            CHUNK_DATA[lod][k].id1 = -1;
            CHUNK_DATA[lod][k].id0 = -1;
            CHUNK_DATA[lod][k].center = job_data[j].center;
            CHUNK_DATA[lod][k].min = job_data[j].min;
            CHUNK_DATA[lod][k].max = job_data[j].max;
            CHUNK_DATA[lod][k].free = false;
            CHUNK_DATA[lod][k].uID = uid;

            already_emitted[lod][k] = true;

            if (lod + 1 < LODMAX) {

                assert(cf >= 0);
                CHUNK_DATA[lod][k].id0 = cf;
                int nc = CHUNK_DATA[lod + 1][cf].id0;
                int pc = CHUNK_DATA[lod + 1][cf].id1;
                CHUNK_DATA[lod + 1][cf].uID = current_chunk;
                CHUNK_DATA[lod + 1][cf].center = job_data[j].center;
                CHUNK_DATA[lod + 1][cf].min = job_data[j].min;
                CHUNK_DATA[lod + 1][cf].max = job_data[j].max;
                CHUNK_DATA[lod + 1][cf].is_child = true;

                CHUNK_DATA[lod + 1][cf].id0 = -1;
                CHUNK_DATA[lod + 1][cf].id1 = k;
                CHUNK_DATA[lod + 1][cf].uID = uid;
                CHUNK_DATA[lod + 1][cf].free = false;
                already_emitted[lod + 1][cf] = true;

                // Since it's a brand new chunk, make a child mesh at higher LOD.
                create_new_planet_mesh(lod+1, job_data[j].center, cf, false);
                replace_chunk_hash(job_data[j].center, lod+1, cf);

            }
            
            replace_chunk_hash(job_data[j].center, lod, k);
            set_current_lod(job_data[j].center, lod);
            is_parent = true;
            chunk_uID++;

        }

        // Create new mesh for given chunk.
        if (k != -1)
        {
            
            create_new_planet_mesh(lod, job_data[j].center, k, is_parent);

            if (is_parent)
            {
                emitted_chunk_ids[emitted_chunks] = k;
                emitted_chunk_lods[emitted_chunks] = lod;
                emitted_chunks++;
            }
        }
    }

    double execute_time = end_timer(&execute);
    zero_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

}


/**
 * @brief Creats a sphere face to be used for atmosphere rendering
 * @param face Which face to generate.
 * @param vcnt The current amount of vertices.
 * @param icnt The current amount of indices.
 * @ingroup atmosphere_group
 */
void
    make_atmosphere_sphere_face
    (
        FACE  face,
        int * vcnt,
        int * icnt
    )

{
    
    int c = *vcnt;
    int o = ATMOSPHERE_VCNT;
    int ic = *icnt;

    Timer check;
    start_timer(&check);
    float scale = 1.0;
    // float size = pow(particle.p_m.w, 1.0/3.0) * scale;

    // float size = 600;
    float radius = (float)(CHUNK_WORLD_SIZE * (CHUNK_RADIUS * 2 + 1) * 2) / 2;
    // float size = radius + radius * 0.2f;
    float size = 1;
    
    for (float y = 0; y < ATMOSPHERE_RES; y+=1.0f)
    {

        for (float x = 0; x < ATMOSPHERE_RES; x+=1.0f)
        {

            vec2 uv = {x / (ATMOSPHERE_RES-1), y /(ATMOSPHERE_RES-1)};
            uv.x = uv.x * 2.0f - 1.0f;
            uv.y = uv.y * 2.0f - 1.0f;
            vec3 p = cube_pos(face, uv);

            p = vec3_normalize(p);

            
            p = vec3_scale(p, size);
        
            ATMOSPHERE_VERTS[ATMOSPHERE_VCNT++].pos = p;

        }
    }

    for (int y = 0; y < ATMOSPHERE_RES-1; y+=1)
    {
        
        for (int x = 0; x < ATMOSPHERE_RES-1; x+=1)
        {

            uint32_t a0 = ATMOSPHERE_IDX(x, y, o);
            uint32_t b0 = ATMOSPHERE_IDX(x+1, y, o);
            uint32_t c0 = ATMOSPHERE_IDX(x, y+1, o);
            ATMOSPHERE_INDICES[ATMOSPHERE_ICNT++] = a0;
            ATMOSPHERE_INDICES[ATMOSPHERE_ICNT++] = b0;
            ATMOSPHERE_INDICES[ATMOSPHERE_ICNT++] = c0;

            vec3 n0 = vec3_normalize(calc_normal(ATMOSPHERE_VERTS[a0].pos, ATMOSPHERE_VERTS[b0].pos, ATMOSPHERE_VERTS[c0].pos));

            ATMOSPHERE_VERTS[a0].normal = vec3_add(ATMOSPHERE_VERTS[a0].normal, n0);
            ATMOSPHERE_VERTS[b0].normal = vec3_add(ATMOSPHERE_VERTS[b0].normal, n0);
            ATMOSPHERE_VERTS[c0].normal = vec3_add(ATMOSPHERE_VERTS[c0].normal, n0);

            uint32_t a1 = ATMOSPHERE_IDX(x, y+1, o);
            uint32_t b1 = ATMOSPHERE_IDX(x+1, y, o);
            uint32_t c1 = ATMOSPHERE_IDX(x+1, y+1, o);
            ATMOSPHERE_INDICES[ATMOSPHERE_ICNT++] = a1;
            ATMOSPHERE_INDICES[ATMOSPHERE_ICNT++] = b1;
            ATMOSPHERE_INDICES[ATMOSPHERE_ICNT++] = c1;

            vec3 n1 = vec3_normalize(calc_normal(ATMOSPHERE_VERTS[a1].pos, ATMOSPHERE_VERTS[b1].pos, ATMOSPHERE_VERTS[c1].pos));

            ATMOSPHERE_VERTS[a1].normal = vec3_add(ATMOSPHERE_VERTS[a1].normal, n1);
            ATMOSPHERE_VERTS[b1].normal = vec3_add(ATMOSPHERE_VERTS[b1].normal, n1);
            ATMOSPHERE_VERTS[c1].normal = vec3_add(ATMOSPHERE_VERTS[c1].normal, n1);
            
        }
    }

    double end = end_timer(&check);


}

/**
 * @brief Creates an atmosphere sphere for rendering.
 * @ingroup atmosphere_group
 */
void
    mesh_atmosphere_sphere ()

{

    for (int face = 0; face < 6; face++) { make_atmosphere_sphere_face(face, &ATMOSPHERE_VCNT, &ATMOSPHERE_ICNT); }

    for (int i = 0; i < ATMOSPHERE_VCNT; i++) { ATMOSPHERE_VERTS[i].normal = vec3_normalize(ATMOSPHERE_VERTS[i].normal); }
    
}



