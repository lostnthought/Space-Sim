#ifndef MATHF_H
#define MATHF_H

#include <math.h>
#include "core/types.h"

static inline vec3 vec3_normalize(vec3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len == 0.0f) return v;
    v.x /= len;
    v.y /= len;
    v.z /= len;

    return v;
}
static inline vec3 ivec3_normalize(ivec3 a)
{
    vec3 v = {(float)a.x, (float)a.y, (float)a.z};
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len == 0.0f) return v;
    v.x /= len;
    v.y /= len;
    v.z /= len;

    return v;
}
static inline vec3 vec3_cross(vec3 a, vec3 b) {
    vec3 out;
    out.x = a.y*b.z - a.z*b.y;
    out.y = a.z*b.x - a.x*b.z;
    out.z = a.x*b.y - a.y*b.x;
    return out;
}

static inline vec2 vec2_scale(vec2 a, float b)
{
    return (vec2){a.x * b, a.y * b};
}
static inline vec3 vec3_scale(vec3 a, float b)
{
    return (vec3){a.x * b, a.y * b, a.z * b};
}
static inline vec4 vec4_scale(vec4 a, float b)
{
    return (vec4){a.x * b, a.y * b, a.z * b, a.w * b};
}

static inline float ivec3_dist(ivec3 a, ivec3 b)
{
    int ix = a.x - b.x, iy = a.y - b.y, iz = a.z - b.z;
    return sqrtf(ix * ix + iy * iy + iz * iz);
}
static inline ivec3 ivec3_sub(ivec3 a, ivec3 b)
{
    return (ivec3){a.x - b.x, a.y - b.y, a.z - b.z};
}
static inline vec3 vec3_sub(vec3 a, vec3 b)
{
    return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}
static inline vec2 vec2_add(vec2 a, vec2 b)
{
    return (vec2){a.x + b.x, a.y + b.y};
}
static inline ivec3 ivec3_add(ivec3 a, ivec3 b)
{
    return (ivec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
static inline vec3 ivec3_scale(ivec3 a, float b)
{
    return (vec3){a.x * b, a.y * b, a.z * b};
}
static inline vec3 vec3_add(vec3 a, vec3 b)
{
    return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
static inline vec4 vec4_add(vec4 a, vec4 b)
{
    return (vec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
static inline vec4 vec4_sub(vec4 a, vec4 b)
{
    return (vec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}
static inline vec3 vec3_mul(vec3 a, vec3 b)
{
    return (vec3){a.x * b.x, a.y * b.y, a.z * b.z};
}
static inline vec4 vec4_mul(vec4 a, vec4 b)
{
    return (vec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}
static inline float vec3_dist2(vec3 a, vec3 b) {
    vec3 d = vec3_sub(a, b);
    return d.x*d.x + d.y*d.y + d.z*d.z;
}
static inline float vec3_dist(vec3 a, vec3 b) {
    vec3 d = vec3_sub(a, b);
    return fabsf(d.x) + fabsf(d.y) + fabsf(d.z);
}
static inline float vec3_dot(vec3 a, vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static inline float vec2_dot(vec2 a, vec2 b) {
    return a.x*b.x + a.y*b.y;
}
static inline float vec3_length(vec3 v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline mat4 mat4_inverse(mat4 in) {
    
    vec3 a = { in.a0, in.a1, in.a2 }; // X basis
    vec3 b = { in.b0, in.b1, in.b2 }; // Y basis
    vec3 c = { in.c0, in.c1, in.c2 }; // Z basis
    vec3 d = { in.d0, in.d1, in.d2 }; // Translation
    
    vec3 cross_bc = vec3_cross(b, c);
    float det = vec3_dot(a, cross_bc);
    float invDet = 1.0f / det;
    vec3 ia = vec3_cross(b, c);
    vec3 ib = vec3_cross(c, a);
    vec3 ic = vec3_cross(a, b);

    ia.x *= invDet; ia.y *= invDet; ia.z *= invDet;
    ib.x *= invDet; ib.y *= invDet; ib.z *= invDet;
    ic.x *= invDet; ic.y *= invDet; ic.z *= invDet;

    vec3 id = {
        -vec3_dot(ia, d),
        -vec3_dot(ib, d),
        -vec3_dot(ic, d)
    };

    mat4 out;
    out.a0=ia.x; out.a1=ia.y; out.a2=ia.z; out.a3=0.0f;
    out.b0=ib.x; out.b1=ib.y; out.b2=ib.z; out.b3=0.0f;
    out.c0=ic.x; out.c1=ic.y; out.c2=ic.z; out.c3=0.0f;
    out.d0=id.x; out.d1=id.y; out.d2=id.z; out.d3=1.0f;

    return out;
}
static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline float clamp(float a, float b, float c){
    
}
static inline vec4 mul_m4_v4(mat4 m, vec4 v) {
    vec4 out;
    out.x = m.a0 * v.x + m.b0 * v.y + m.c0 * v.z + m.d0 * v.w;
    out.y = m.a1 * v.x + m.b1 * v.y + m.c1 * v.z + m.d1 * v.w;
    out.z = m.a2 * v.x + m.b2 * v.y + m.c2 * v.z + m.d2 * v.w;
    out.w = m.a3 * v.x + m.b3 * v.y + m.c3 * v.z + m.d3 * v.w;
    return out;
}

static inline mat4 m4_mul(mat4 a, mat4 b) {
    mat4 out;
    vec4 b0 = { b.a0, b.a1, b.a2, b.a3 };
    vec4 b1 = { b.b0, b.b1, b.b2, b.b3 };
    vec4 b2 = { b.c0, b.c1, b.c2, b.c3 };
    vec4 b3 = { b.d0, b.d1, b.d2, b.d3 };

    vec4 r0 = mul_m4_v4(a, b0);
    vec4 r1 = mul_m4_v4(a, b1);
    vec4 r2 = mul_m4_v4(a, b2);
    vec4 r3 = mul_m4_v4(a, b3);

    out.a0 = r0.x; out.a1 = r0.y; out.a2 = r0.z; out.a3 = r0.w;
    out.b0 = r1.x; out.b1 = r1.y; out.b2 = r1.z; out.b3 = r1.w;
    out.c0 = r2.x; out.c1 = r2.y; out.c2 = r2.z; out.c3 = r2.w;
    out.d0 = r3.x; out.d1 = r3.y; out.d2 = r3.z; out.d3 = r3.w;

    return out;
}

static inline void mat4_mul(float out[16], const float a[16], const float b[16]) {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
}

#define MAX(a, b) (a > b ? (a) : (b))

#define MIN(a, b) (a < b ? (a) : (b))


static inline uint32_t expand_bits(uint32_t v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

static inline uint32_t morton(uint32_t x, uint32_t y, uint32_t z)
{
    return expand_bits(x) | (expand_bits(y) << 1) | (expand_bits(z) << 2);
}

#endif
