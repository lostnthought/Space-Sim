#version 430 core

in vec2 vUV;

// uniform sampler2D uTexture;
uniform sampler2D uScene;
uniform sampler2D uDepth;
uniform isampler2D uID;
// uniform sampler2D uUI;
// uniform float uTime;

uniform sampler2D bloom;
uniform sampler2D bloom2;
uniform sampler2D history;
uniform sampler2D shadow_map;
uniform sampler2D uWater;
uniform vec3 cam_right;
uniform int uHovered;
uniform int uSelected;
uniform vec2 uTexel;

#define BODY_COUNT 15000
#define MAX_NODES 45000
layout(std140) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
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


layout(std430, binding=6) buffer NodeCounter {
    uint queue_out;
    int p_dead_cnt;
    int p_spawn_cnt;
    uint star_pcnt;
};

uniform int outside_atmosphere;
out vec4 fragColor;
const vec2 res = vec2(1920, 1080);
float linearize(float z)
{
    float near = 1.0;
    float far = 5.0;
    return (2.0 * near) / (far + near - z * (far - near));
}
void main()
{
    vec2 uv = vUV;
    float closest = 1000.0;
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

    int middle_id = uHovered;
    int selected = uSelected;
	int id = texture(uID, uv).r;



    
    // for (int i = 0; i < star_pcnt; i++)
    // {
    //     vec4 starPos = p[star_particles[i]].p_m;

    //     vec4 clip = projection * view * vec4(starPos.xyz,1.0);
    //     vec2 starScreen = clip.xy / clip.w;
    //     starScreen = starScreen * 0.5 + 0.5;
    //     float starDepth = clip.z / clip.w;
    //     starDepth = starDepth * 0.5 + 0.5;
    //     vec2 delta = uv - starScreen;
    //     if (starDepth < closest)
    //     {
    //         closest = starDepth;
    //     }
    //     else
    //     {
    //         continue;
    //     }
    //     float sceneDepth = texture(uDepth, uv).r;

    //     // if (sceneDepth < starDepth) continue;
    //     float weight = starPos.w;
    //     float massRadius = pow(weight, 0.5);

    //     float scale = 1.0f;
    //     float size = pow(weight, 1.0/3.0) * scale;
    //     // float size = massRadius;
    //     // size = clamp(size, 0.1, 50.0);

    //     vec4 c = projection * view * vec4(starPos.xyz,1.0);
    //     vec4 e = projection * view * vec4(starPos.xyz + cam_right * size,1.0);

    //     vec2 center = c.xy / c.w;
    //     vec2 edge = e.xy / e.w;
    //     center *= 0.5 + 0.5;
    //     edge *= 0.5 + 0.5;

    //     float screenRadius = length(edge - center) * 0.5;


    //     delta.x *= res.x / res.y;
    //     float mass = 0.2;
    //     float r = length(delta);
    //     if (r < screenRadius) continue;
    //     float rnorm = r / screenRadius;
        
    //     float alpha = mass* 5/(rnorm* rnorm + 1.0);

    //     float mask = smoothstep(1.0, 2.0, rnorm);
    //     // float streak = pow(abs(dot(normalize(delta), vec2(1,1))), 32.0);
    //     // color += p_colors[star_particles[i]] * streak * 0.01 * alpha;

    //     uv += alpha * 0.05 * normalize(delta) * mask;

    // }


    vec4 sc = texture(uScene, uv);
    vec4 wc = texture(uWater, uv);
    float mask = float(wc.a < sc.a);
    if (outside_atmosphere == 0)
    {
        color.rgb += mix(sc.rgb, wc.rgb, mask);
    }
    else
    {
        color.rgb += sc.rgb;
    }
    float abb_amt = 0.005;
    color.rgb = pow(color.rgb, vec3(0.9));
    color.rgb = clamp((color.rgb - 0.15) * 1.4 + 0.1, 0.0, 1.0);
    color.rgb *= vec3(1.05, 0.95, 1.1);

    float gamma = 2.2;
    color.rgb = pow(color.rgb, vec3(1.0/gamma));

    fragColor = vec4(color.rgb, 1.0);
}
