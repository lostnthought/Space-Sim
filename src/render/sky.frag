#version 460

layout(location = 0)out vec4 fragColor;



in vec3 vPos;
uniform samplerCube uCubemap;
const int size = 128;
void main()
{

    vec3 pos = vPos;
    fragColor = texture(uCubemap, pos);

}
