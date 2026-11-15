
#version 440

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform float uPlanetRadius;

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
void main()
{
    
    vec3 pos = vertices[gl_VertexID].pos;
    pos.z += uPlanetRadius;
    
    vec4 worldPos = projection * view * vec4(pos, 1.0);
    gl_Position = worldPos; 
}
