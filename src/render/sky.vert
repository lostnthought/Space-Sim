
#version 460

layout(location = 0) in vec3 aPosition;

layout(std140) uniform CameraData {
    vec4 cam_pos;
    vec4 cam_vel;
    mat4 projection;
    mat4 view;
    mat4 inv_projection;
    mat4 inv_view;
};

uniform mat4 light_view;
uniform mat4 light_proj;

out vec3 vPos;
void main()
{
    

    float c = cos(1.0f);
    
    float s = sin(1.0f);

    mat3 rotation = mat3(
        vec3(1, 0, 0),
        vec3(0, 0, 1),
        vec3(0, -1, 0)
    );
    
    vec3 pos = aPosition;
    vPos = pos;
    pos = rotation * pos;
    vec4 p = projection * view * vec4(pos, 0.0); 

    gl_Position = p.xyww;
    
}
