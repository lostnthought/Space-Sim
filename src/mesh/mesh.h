#ifndef MESH_H
#define MESH_H

#include "core/types.h"
#include "uthash/uthash.h"
#include <stdio.h>


typedef struct {
    int x, y, z;
} CenterKey;

typedef struct {
    CenterKey key;
    int lod;
    UT_hash_handle hh;
} AuthorityEntry;



typedef struct ChunkKey
{

    int x, y, z, lod;

} ChunkKey;

typedef struct ChunkHash
{

    ChunkKey key;
    int id;
    UT_hash_handle hh;

} ChunkHash;
void load_chunks(vec3 planet_origin, float planet_radius);

void init_chunk_data();
bool free_parent(int lod, int i);
bool free_child(int lod, int i);
bool free_current_parent(int lod, int i);

float calc_density(vec3 p);
void mesh_atmosphere_sphere();
void create_new_planet_mesh(int lod, ivec3 chunk_center, int chunk_id, bool is_parent);
float fbm(vec3 p);

extern AuthorityEntry * authority_map;
extern ChunkHash * hash_table;

static inline int get_current_lod(ivec3 center)
{
    AuthorityEntry* e;
    CenterKey k = {center.x, center.y, center.z};

    HASH_FIND(hh, authority_map, &k, sizeof(k), e);
    if (!e) return -1;

    return e->lod;
}

static inline void set_current_lod(ivec3 center, int lod)
{
    AuthorityEntry* e = NULL;
    CenterKey k = {center.x, center.y, center.z};

    HASH_FIND(hh, authority_map, &k, sizeof(k), e);

    if (!e) {
        e = (AuthorityEntry*)malloc(sizeof(*e));
        e->key = k;
        HASH_ADD(hh, authority_map, key, sizeof(k), e);
    }

    e->lod = lod;
}

// find by center, return even if lod does not match
static inline ChunkHash * find_chunk_hash(ivec3 center, int lod)
{
    ChunkHash * f = NULL;
       
    ChunkKey k = {(center.x), (center.y), (center.z), lod};
    HASH_FIND(hh, hash_table, &k, sizeof(ChunkKey), f);

    return f;
}
static inline void add_chunk_hash(ivec3 center, int lod, int id)
{
    
    ChunkKey k = {(center.x), (center.y), (center.z), lod};
    ChunkHash *a = (ChunkHash*)malloc(sizeof(ChunkHash));
    memset(a, 0, sizeof(ChunkHash));
    a->key = k;
    a->id = id;
    // a->hh = hh;
    HASH_ADD(hh, hash_table, key, sizeof(ChunkKey), a);

}

static inline void replace_chunk_hash(ivec3 center, int lod, int id)
{

    ChunkKey k = {(center.x), (center.y), (center.z), lod};
    // a->hh = hh;
    ChunkHash * r = NULL;
    // HASH_REPLACE(hh, hash_table, key, sizeof(ChunkKey), a, r);
    HASH_FIND(hh, hash_table, &k, sizeof(k), r);

    if (!r) {
        ChunkHash *a = (ChunkHash*)malloc(sizeof(ChunkHash));
        memset(a, 0, sizeof(ChunkHash));
        a->key = k;
        a->id = id;
        HASH_ADD(hh, hash_table, key, sizeof(ChunkKey), a);
    }
    else
    {
        r->id = id;
        r->key = k;
    }

    
}

static inline void delete_chunk_hash(ivec3 center, int lod)
{
    
    ChunkHash * p = NULL;
    ChunkKey k = {(center.x), (center.y), (center.z), lod};
    HASH_FIND(hh, hash_table, &k, sizeof(ChunkKey), p);
    if (p)
    {
        HASH_DEL(hash_table, p);
        free(p);
    }
    else
    {
        // printf("NO DELETION HASH FOUND %d %d %d\n", center.x, center.y, center.z);
        // exit(0);
    }
}
#endif
