#version 460

/**
 * @file water.frag
 * @ingroup water_group
 * @brief The shader that handles water shading and effects.
 */
layout(location = 0)out vec4 fragColor;
layout(std140, binding = 0) uniform CameraData {
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
in vec3 vPos;
in vec3 vNormal;
in vec3 rvPos;
in vec3 rvNormal;
in float vDist;
in vec2 vUV;
in vec3 vViewDir;
uniform mat4 light_proj;
uniform mat4 light_view;
in vec4 vFragPosLightSpace;

// out vec4 fragColor;



#define MAX_LAYERS 24

struct WarpLayer
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
};

struct PerlinLayer
{
    float freq;
    float amp;
    float pad0;
    float pad1;
};
    
struct FBMLayer
{
    float freq;
    float amp;
    float pad0;
    float pad1;
};
    
layout(std430, binding=13) buffer PlanetData {

    int warp_count;
    int perlin_count;
    int fbm_count;
    int pad0;
    vec4 sun_dir;
    vec4 sun_color;
    WarpLayer warp_layers[MAX_LAYERS];
    PerlinLayer perlin_layers[MAX_LAYERS];
    FBMLayer fbm_layers[MAX_LAYERS];

};


const vec3 water_color = vec3(0.01, 0.1, 0.3);
const vec3 sky_color = vec3(0.3, 0.3, 0.5);

#define N 128

uniform sampler2D tex;
uniform sampler2D uFFT;
uniform sampler2D uSlope;
uniform sampler2D uSky;
uniform sampler2D normal_map;
uniform samplerCube uCubemap;
uniform float uTime;
uniform sampler2D uDepth;
uniform sampler2D uScene;
uniform sampler2D uExp;
uniform sampler3D fbm_tex;
uniform sampler2D shadow_map;

const vec3 tX = vec3(0, 0, 1);
const vec3 bX = vec3(0, 1, 0);
const vec3 nXw = vec3(1, 0, 0);

const vec3 tY = vec3(1, 0, 0);
const vec3 bY = vec3(0, 0, 1);
const vec3 nYw = vec3(0, 1, 0);

const vec3 tZ = vec3(1, 0, 0);
const vec3 bZ = vec3(0, 1, 0);
const vec3 nZw = vec3(0, 0, 1);
const int water_scale = 128;

float getSun(vec3 dir, vec3 ld) { 
  return pow(max(0.0, dot(dir, normalize(ld))), 720.0) * 210.0;
}

int L = 2000;
int NUM_TILES_PER_WATER_SCALE = 4;

vec4 sampleFFT(sampler2D tex, vec2 uv)
{
    vec2 texel = 1.0 / textureSize(tex, 0);

    vec4 c = texture(tex, uv) * 0.5;
    c += texture(tex, uv + vec2(texel.x, 0)) * 0.125;
    c += texture(tex, uv - vec2(texel.x, 0)) * 0.125;
    c += texture(tex, uv + vec2(0, texel.y)) * 0.125;
    c += texture(tex, uv - vec2(0, texel.y)) * 0.125;

    return c;
}
float linearize(float z)
{
    float near = 0.01;
    float far = 100000.0;
    return (2.0 * near) / (far + near - z * (far - near));
}


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





float shadow_calculation(vec4 pos, float diff)
{
    vec3 proj_coords = pos.xyz / pos.w;
    proj_coords = proj_coords * 0.5 + 0.5;
    float closest_depth = texture(shadow_map, proj_coords.xy).r;
    float current_depth = proj_coords.z;


    float bias = max(0.05f * (diff), 0.005f);
    float shadow = 0.0f;
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
    for (int y = -1; y <= 1; y++)
    {
        
        for (int x = -1; x <= 1; x++)
        {

            float pcfDepth = texture(shadow_map, proj_coords.xy + vec2(x, y) * texel_size).r;

            shadow += (current_depth - bias > pcfDepth ? 1.0 : 0.0); 
        }
    }
    shadow /= 9;
    if (proj_coords.z > 1.0) shadow = 0.0;
    
    return shadow;
}


void main()
{

    vec2 uv = vUV;
    vec3 n = vNormal;
    vec2 screenUV = gl_FragCoord.xy / resolution.xy;
    vec4 sc_offset = texture(uScene, screenUV + n.xy * 0.02);
    float waterDepth = linearize(gl_FragCoord.z);
    float depth_diff = clamp((sc_offset.a - waterDepth) * 1000, 0, 500);
    float absorption_coeff = 0.2;
    float absorption = exp2((-depth_diff * absorption_coeff) * 1.442695);
    vec3 view_dir = vViewDir;
    float f = 1.0 - clamp(dot(n, view_dir), 0.0, 1.0);
    float fresnel = f * f;
    fresnel *= fresnel;


    vec3 ld = sun_dir.xyz;
    ld = normalize(-ld);

    float diffuse = max(dot(n, ld), 0.0);

    vec3 halfway = normalize(ld + view_dir);
    float ndh = max(dot(n, halfway), 0.0);
    float r = 0.05;
    float spec = ndh * ndh;
    spec *= spec;
    spec *= spec;
    vec3 R = reflect(-view_dir, n);


    R.z = abs(R.z);
    mat3 rotation = mat3(
        vec3(1, 0, 0),
        vec3(0, 0, 1),
        vec3(0, -1, 0)
    );
    R *= rotation;
    vec3 reflection = texture(uCubemap, R).rgb;
    vec3 refraction = mix(water_color, sc_offset.rgb, 0.1);
    float thin = smoothstep(0.7, 1.0, 1.0 - dot(n, vec3(0,0,1)));
    vec3 thinColor = vec3(0.1, 0.5, 0.3);

    refraction += thin * thinColor;

    reflection += spec * sun_color.rgb;

    float shadow = shadow_calculation(vFragPosLightSpace, diffuse);
    vec3 color = refraction * (1-fresnel) + reflection * fresnel;

    fragColor = vec4(color * (1-shadow), waterDepth);
}
