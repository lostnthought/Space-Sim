
#include "core/init.h"
#include "core/shaders.h"
#include <stdio.h>
#include <string.h>

char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buffer = (char *)malloc(size + 1);
    if (!buffer) return NULL;
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}
GLuint create_compute_shader(const char *filename) {
    char *src = read_file(filename);
    if (!src) {
        fprintf(stderr, "Failed to read shader file: %s\n", filename);
        return 0;
    }

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, (const GLchar * const *)&src, NULL);
    glCompileShader(shader);
    free(src); // don't forget to free it after compile

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Compute Shader Error %s: %s\n", filename, log);
        exit(0);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    GLint status2;
    glGetProgramiv(program, GL_LINK_STATUS, &status2);
    if (!status2) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        fprintf(stderr, "Compute Shader LINKER Error %s: %s\n", filename, log);
        exit(0);
        return 0;
    }

    glDeleteShader(shader);
    return program;
}


GLuint compile_shader(GLenum type, const char *filename) {
    const char *source = read_file(filename);
    if (!source) return 0;

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    free((void *)source);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    char log[512];
    if (!success) {
        
        glGetShaderInfoLog(shader, 512, NULL, log);
        printf("\\nShader compile error (%s):\\n%s\\n", filename, log);
        exit(0);
        return 0;
    } else {
        glGetShaderInfoLog(shader, 512, NULL, log);
        if (strlen(log) > 0) {
            printf("Shader compile warning (%s):\\n%s\\n", filename, log);
        }
    }

    return shader;
}



GLuint create_shader_program(const char *vertexPath, const char *fragmentPath) {
    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentPath);
    if (!vertexShader || !fragmentShader) return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    char log[512];
    if (!success) {
        
        glGetProgramInfoLog(program, 512, NULL, log);
        printf("Shader link error:\\n%s\\n", log);
        exit(0);
        return 0;
    } else {
        glGetProgramInfoLog(program, 512, NULL, log);
        if (strlen(log) > 0) {
            printf("Shader link warning:\\n%s\\n", log);
        }
}

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}






void init_main_shader()
{
    sprintf(MAIN_SHADER.vert_path, "./src/render/main.vert");
    sprintf(MAIN_SHADER.frag_path, "./src/render/main.frag");

    MAIN_SHADER.program = create_shader_program(MAIN_SHADER.vert_path, MAIN_SHADER.frag_path);

    MAIN_SHADER.uniforms[0].location = glGetUniformLocation(MAIN_SHADER.program, "uSphere");
    sprintf(MAIN_SHADER.uniforms[0].name, "uSphere");
    MAIN_SHADER.uniforms[1].location = glGetUniformLocation(MAIN_SHADER.program, "cam_pos");
    sprintf(MAIN_SHADER.uniforms[1].name, "cam_pos");
    MAIN_SHADER.uniforms[2].location = glGetUniformLocation(MAIN_SHADER.program, "cam_right");
    sprintf(MAIN_SHADER.uniforms[2].name, "cam_right");
    MAIN_SHADER.uniforms[3].location = glGetUniformLocation(MAIN_SHADER.program, "cam_up");
    sprintf(MAIN_SHADER.uniforms[3].name, "cam_up");
    MAIN_SHADER.uniforms[4].location = glGetUniformLocation(MAIN_SHADER.program, "cam_forward");
    sprintf(MAIN_SHADER.uniforms[4].name, "cam_forward");

}


void init_mesh_shader()
{

    sprintf(MESH_SHADER.vert_path, "./src/render/mesh.vert");
    sprintf(MESH_SHADER.frag_path, "./src/render/mesh.frag");

    MESH_SHADER.program = create_shader_program(MESH_SHADER.vert_path, MESH_SHADER.frag_path);


    MESH_SHADER.uniforms[0].location = glGetUniformLocation(MESH_SHADER.program, "uSelected");
    sprintf(MESH_SHADER.uniforms[0].name, "uSelected");
    MESH_SHADER.uniforms[1].location = glGetUniformLocation(MESH_SHADER.program, "cam_pos");
    sprintf(MESH_SHADER.uniforms[1].name, "cam_pos");
    MESH_SHADER.uniforms[2].location = glGetUniformLocation(MESH_SHADER.program, "uTime");
    sprintf(MESH_SHADER.uniforms[2].name, "uTime");
    MESH_SHADER.uniforms[3].location = glGetUniformLocation(MESH_SHADER.program, "perlin");
    sprintf(MESH_SHADER.uniforms[3].name, "perlin");
    MESH_SHADER.uniforms[4].location = glGetUniformLocation(MESH_SHADER.program, "normal_map");
    sprintf(MESH_SHADER.uniforms[4].name, "normal_map");
    MESH_SHADER.uniforms[5].location = glGetUniformLocation(MESH_SHADER.program, "diffuse_tex");
    sprintf(MESH_SHADER.uniforms[5].name, "diffuse_tex");
    MESH_SHADER.uniforms[6].location = glGetUniformLocation(MESH_SHADER.program, "spec_tex");
    sprintf(MESH_SHADER.uniforms[6].name, "spec_tex");
    MESH_SHADER.uniforms[7].location = glGetUniformLocation(MESH_SHADER.program, "ao_tex");
    sprintf(MESH_SHADER.uniforms[7].name, "ao_tex");
    MESH_SHADER.uniforms[8].location = glGetUniformLocation(MESH_SHADER.program, "shadow_map");
    sprintf(MESH_SHADER.uniforms[8].name, "shadow_map");
    MESH_SHADER.uniforms[9].location = glGetUniformLocation(MESH_SHADER.program, "light_proj");
    sprintf(MESH_SHADER.uniforms[9].name, "light_proj");
    MESH_SHADER.uniforms[10].location = glGetUniformLocation(MESH_SHADER.program, "light_view");
    sprintf(MESH_SHADER.uniforms[10].name, "light_view");
    
    MESH_SHADER.uniforms[11].location = glGetUniformLocation(MESH_SHADER.program, "uType");
    sprintf(MESH_SHADER.uniforms[11].name, "uType");
    MESH_SHADER.uniforms[12].location = glGetUniformLocation(MESH_SHADER.program, "lod_level");
    sprintf(MESH_SHADER.uniforms[12].name, "lod_level");
}
void init_atmosphere_shader()
{

    sprintf(ATMOSPHERE_SHADER.vert_path, "./src/render/atmosphere.vert");
    sprintf(ATMOSPHERE_SHADER.frag_path, "./src/render/atmosphere.frag");

    ATMOSPHERE_SHADER.program = create_shader_program(ATMOSPHERE_SHADER.vert_path, ATMOSPHERE_SHADER.frag_path);


    // camera height ** 2
    // inner radius
    // inner radius ** 2
    // outer radius
    // outer radius ** 2
    // inv wavelength -> scaled = 5.5, 13.0, 22.4
    // fscale -> 1 / (outer radius - inner radius)
    // fscale / fscaledepth
    // g
    // g ** 2
    ATMOSPHERE_SHADER.uniforms[0].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uCamPosWorld");
    sprintf(ATMOSPHERE_SHADER.uniforms[0].name, "uCamPosWorld");
    ATMOSPHERE_SHADER.uniforms[1].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uInnerRadius");
    sprintf(ATMOSPHERE_SHADER.uniforms[1].name, "uInnerRadius");
    ATMOSPHERE_SHADER.uniforms[2].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uInnerRadius2");
    sprintf(ATMOSPHERE_SHADER.uniforms[2].name, "uInnerRadius2");
    ATMOSPHERE_SHADER.uniforms[3].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uOuterRadius");
    sprintf(ATMOSPHERE_SHADER.uniforms[3].name, "uOuterRadius");
    ATMOSPHERE_SHADER.uniforms[4].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uOuterRadius2");
    sprintf(ATMOSPHERE_SHADER.uniforms[4].name, "uOuterRadius2");
    ATMOSPHERE_SHADER.uniforms[5].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uInvWavelength");
    sprintf(ATMOSPHERE_SHADER.uniforms[5].name, "uInvWavelength");
    ATMOSPHERE_SHADER.uniforms[6].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uFScale");
    sprintf(ATMOSPHERE_SHADER.uniforms[6].name, "uFScale");
    ATMOSPHERE_SHADER.uniforms[7].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uFScaleOverDepth");
    sprintf(ATMOSPHERE_SHADER.uniforms[7].name, "uFScaleOverDepth");
    ATMOSPHERE_SHADER.uniforms[8].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uFInvScaleDepth");
    sprintf(ATMOSPHERE_SHADER.uniforms[8].name, "uFInvScaleDepth");
    ATMOSPHERE_SHADER.uniforms[9].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uFScaleDepth");
    sprintf(ATMOSPHERE_SHADER.uniforms[9].name, "uFScaleDepth");
    ATMOSPHERE_SHADER.uniforms[10].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uG");
    sprintf(ATMOSPHERE_SHADER.uniforms[10].name, "uG");
    ATMOSPHERE_SHADER.uniforms[11].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uG2");
    sprintf(ATMOSPHERE_SHADER.uniforms[11].name, "uG2");
    ATMOSPHERE_SHADER.uniforms[12].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "ufKrESun");
    sprintf(ATMOSPHERE_SHADER.uniforms[12].name, "ufKrESun");
    ATMOSPHERE_SHADER.uniforms[13].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "ufKmESun");
    sprintf(ATMOSPHERE_SHADER.uniforms[13].name, "ufKmESun");
    ATMOSPHERE_SHADER.uniforms[14].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "ufKr4Pi");
    sprintf(ATMOSPHERE_SHADER.uniforms[14].name, "ufKr4Pi");
    ATMOSPHERE_SHADER.uniforms[15].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "ufKm4Pi");
    sprintf(ATMOSPHERE_SHADER.uniforms[15].name, "ufKm4Pi");
    ATMOSPHERE_SHADER.uniforms[16].location = glGetUniformLocation(ATMOSPHERE_SHADER.program, "uFakeView");
    sprintf(ATMOSPHERE_SHADER.uniforms[16].name, "uFakeView");
}
void init_clouds_shader()
{

    sprintf(CLOUDS_SHADER.vert_path, "./src/render/clouds.vert");
    sprintf(CLOUDS_SHADER.frag_path, "./src/render/clouds.frag");

    CLOUDS_SHADER.program = create_shader_program(CLOUDS_SHADER.vert_path, CLOUDS_SHADER.frag_path);
    CLOUDS_SHADER.uniforms[0].location = glGetUniformLocation(CLOUDS_SHADER.program, "uCamPosWorld");
    sprintf(CLOUDS_SHADER.uniforms[0].name, "uCamPosWorld");
    CLOUDS_SHADER.uniforms[1].location = glGetUniformLocation(CLOUDS_SHADER.program, "uInnerRadius");
    sprintf(CLOUDS_SHADER.uniforms[1].name, "uInnerRadius");
    CLOUDS_SHADER.uniforms[2].location = glGetUniformLocation(CLOUDS_SHADER.program, "perlin_tex");
    sprintf(CLOUDS_SHADER.uniforms[2].name, "perlin_tex");
    CLOUDS_SHADER.uniforms[3].location = glGetUniformLocation(CLOUDS_SHADER.program, "fbm_tex");
    sprintf(CLOUDS_SHADER.uniforms[3].name, "fbm_tex");
    CLOUDS_SHADER.uniforms[4].location = glGetUniformLocation(CLOUDS_SHADER.program, "uFakeProj");
    sprintf(CLOUDS_SHADER.uniforms[4].name, "uFakeProj");
    CLOUDS_SHADER.uniforms[5].location = glGetUniformLocation(CLOUDS_SHADER.program, "uFakeView");
    sprintf(CLOUDS_SHADER.uniforms[5].name, "uFakeView");
}

void init_atmosphere_from_space_shader()
{

    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.vert_path, "./src/render/atmosphere_from_space.vert");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.frag_path, "./src/render/atmosphere.frag");

    ATMOSPHERE_FROM_SPACE_SHADER.program = create_shader_program(ATMOSPHERE_FROM_SPACE_SHADER.vert_path, ATMOSPHERE_FROM_SPACE_SHADER.frag_path);


    // camera height ** 2
    // inner radius
    // inner radius ** 2
    // outer radius
    // outer radius ** 2
    // inv wavelength -> scaled = 5.5, 13.0, 22.4
    // fscale -> 1 / (outer radius - inner radius)
    // fscale / fscaledepth
    // g
    // g ** 2
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[0].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uCamPosWorld");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[0].name, "uCamPosWorld");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[1].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uInnerRadius");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[1].name, "uInnerRadius");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[2].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uInnerRadius2");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[2].name, "uInnerRadius2");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[3].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uOuterRadius");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[3].name, "uOuterRadius");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[4].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uOuterRadius2");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[4].name, "uOuterRadius2");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[5].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uInvWavelength");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[5].name, "uInvWavelength");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[6].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uFScale");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[6].name, "uFScale");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[7].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uFScaleOverDepth");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[7].name, "uFScaleOverDepth");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[8].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uFInvScaleDepth");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[8].name, "uFInvScaleDepth");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[9].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uFScaleDepth");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[9].name, "uFScaleDepth");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[10].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uG");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[10].name, "uG");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[11].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uG2");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[11].name, "uG2");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[12].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "ufKrESun");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[12].name, "ufKrESun");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[13].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "ufKmESun");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[13].name, "ufKmESun");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[14].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "ufKr4Pi");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[14].name, "ufKr4Pi");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[15].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "ufKm4Pi");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[15].name, "ufKm4Pi");
    ATMOSPHERE_FROM_SPACE_SHADER.uniforms[16].location = glGetUniformLocation(ATMOSPHERE_FROM_SPACE_SHADER.program, "uFakeView");
    sprintf(ATMOSPHERE_FROM_SPACE_SHADER.uniforms[16].name, "uFakeView");
}
void init_sky_shader()
{
    
    sprintf(SKY_SHADER.vert_path, "./src/render/sky.vert");
    sprintf(SKY_SHADER.frag_path, "./src/render/sky.frag");

    SKY_SHADER.program = create_shader_program(SKY_SHADER.vert_path, SKY_SHADER.frag_path);

    SKY_SHADER.uniforms[0].location = glGetUniformLocation(SKY_SHADER.program, "uCubemap");
    sprintf(SKY_SHADER.uniforms[0].name, "uCubemap");

}


void init_water_shader()
{
    sprintf(WATER_SHADER.vert_path, "./src/render/water.vert");
    sprintf(WATER_SHADER.frag_path, "./src/render/water.frag");

    WATER_SHADER.program = create_shader_program(WATER_SHADER.vert_path, WATER_SHADER.frag_path);

    WATER_SHADER.uniforms[0].location = glGetUniformLocation(WATER_SHADER.program, "uTime");
    sprintf(WATER_SHADER.uniforms[0].name, "uTime");
    WATER_SHADER.uniforms[1].location = glGetUniformLocation(WATER_SHADER.program, "tex");
    sprintf(WATER_SHADER.uniforms[1].name, "tex");
    WATER_SHADER.uniforms[2].location = glGetUniformLocation(WATER_SHADER.program, "normal_map");
    sprintf(WATER_SHADER.uniforms[2].name, "normal_map");
    WATER_SHADER.uniforms[3].location = glGetUniformLocation(WATER_SHADER.program, "uCubemap");
    sprintf(WATER_SHADER.uniforms[3].name, "uCubemap");
    WATER_SHADER.uniforms[4].location = glGetUniformLocation(WATER_SHADER.program, "uFFT");
    sprintf(WATER_SHADER.uniforms[4].name, "uFFT");
    WATER_SHADER.uniforms[5].location = glGetUniformLocation(WATER_SHADER.program, "uSky");
    sprintf(WATER_SHADER.uniforms[5].name, "uSky");
    WATER_SHADER.uniforms[6].location = glGetUniformLocation(WATER_SHADER.program, "uSlope");
    sprintf(WATER_SHADER.uniforms[6].name, "uSlope");
    WATER_SHADER.uniforms[7].location = glGetUniformLocation(WATER_SHADER.program, "shadow_map");
    sprintf(WATER_SHADER.uniforms[7].name, "shadow_map");
    WATER_SHADER.uniforms[8].location = glGetUniformLocation(WATER_SHADER.program, "light_proj");
    sprintf(WATER_SHADER.uniforms[8].name, "light_proj");
    WATER_SHADER.uniforms[9].location = glGetUniformLocation(WATER_SHADER.program, "light_view");
    sprintf(WATER_SHADER.uniforms[9].name, "light_view");
    
}


void init_shadow_map_shader()
{

    sprintf(SHADOW_MAP_SHADER.vert_path, "./src/render/shadow.vert");
    sprintf(SHADOW_MAP_SHADER.frag_path, "./src/render/shadow.frag");

    SHADOW_MAP_SHADER.program = create_shader_program(SHADOW_MAP_SHADER.vert_path, SHADOW_MAP_SHADER.frag_path);

    SHADOW_MAP_SHADER.uniforms[0].location = glGetUniformLocation(SHADOW_MAP_SHADER.program, "projection");
    sprintf(SHADOW_MAP_SHADER.uniforms[0].name, "projection");
    SHADOW_MAP_SHADER.uniforms[1].location = glGetUniformLocation(SHADOW_MAP_SHADER.program, "view");
    sprintf(SHADOW_MAP_SHADER.uniforms[1].name, "view");
    
}
void init_shadow_water_shader()
{
    sprintf(SHADOW_WATER_SHADER.vert_path, "./src/render/water.vert");
    sprintf(SHADOW_WATER_SHADER.frag_path, "./src/render/shadow.frag");

    SHADOW_WATER_SHADER.program = create_shader_program(SHADOW_WATER_SHADER.vert_path, SHADOW_WATER_SHADER.frag_path);

    SHADOW_WATER_SHADER.uniforms[0].location = glGetUniformLocation(SHADOW_WATER_SHADER.program, "uTime");
    sprintf(SHADOW_WATER_SHADER.uniforms[0].name, "uTime");
    SHADOW_WATER_SHADER.uniforms[1].location = glGetUniformLocation(SHADOW_WATER_SHADER.program, "tex");
    sprintf(SHADOW_WATER_SHADER.uniforms[1].name, "tex");
    SHADOW_WATER_SHADER.uniforms[2].location = glGetUniformLocation(SHADOW_WATER_SHADER.program, "normal_map");
    sprintf(SHADOW_WATER_SHADER.uniforms[2].name, "normal_map");
    SHADOW_WATER_SHADER.uniforms[3].location = glGetUniformLocation(SHADOW_WATER_SHADER.program, "uCubemap");
    sprintf(SHADOW_WATER_SHADER.uniforms[3].name, "uCubemap");
    SHADOW_WATER_SHADER.uniforms[4].location = glGetUniformLocation(SHADOW_WATER_SHADER.program, "uFFT");
    sprintf(SHADOW_WATER_SHADER.uniforms[4].name, "uFFT");
    SHADOW_WATER_SHADER.uniforms[5].location = glGetUniformLocation(SHADOW_WATER_SHADER.program, "uSky");
    sprintf(SHADOW_WATER_SHADER.uniforms[5].name, "uSky");
    SHADOW_WATER_SHADER.uniforms[6].location = glGetUniformLocation(SHADOW_WATER_SHADER.program, "uSlope");
    sprintf(SHADOW_WATER_SHADER.uniforms[6].name, "uSlope");
    
}


void init_postprocess_shader()
{

    sprintf(POSTPROCESS_SHADER.vert_path, "./src/render/postprocess.vert");
    sprintf(POSTPROCESS_SHADER.frag_path, "./src/render/postprocess.frag");

    POSTPROCESS_SHADER.program = create_shader_program(POSTPROCESS_SHADER.vert_path, POSTPROCESS_SHADER.frag_path);


    POSTPROCESS_SHADER.uniforms[0].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "uScene");
    sprintf(POSTPROCESS_SHADER.uniforms[0].name, "uScene");
    POSTPROCESS_SHADER.uniforms[1].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "uID");
    sprintf(POSTPROCESS_SHADER.uniforms[1].name, "uID");
    POSTPROCESS_SHADER.uniforms[2].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "uDepth");
    sprintf(POSTPROCESS_SHADER.uniforms[2].name, "uDepth");
    POSTPROCESS_SHADER.uniforms[3].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "bloom");
    sprintf(POSTPROCESS_SHADER.uniforms[3].name, "bloom");
    POSTPROCESS_SHADER.uniforms[4].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "bloom2");
    sprintf(POSTPROCESS_SHADER.uniforms[4].name, "bloom2");
    POSTPROCESS_SHADER.uniforms[5].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "history");
    sprintf(POSTPROCESS_SHADER.uniforms[5].name, "history");
    POSTPROCESS_SHADER.uniforms[6].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "shadow_map");
    sprintf(POSTPROCESS_SHADER.uniforms[6].name, "shadow_map");
    POSTPROCESS_SHADER.uniforms[7].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "uHovered");
    sprintf(POSTPROCESS_SHADER.uniforms[7].name, "uHovered");
    POSTPROCESS_SHADER.uniforms[8].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "uSelected");
    sprintf(POSTPROCESS_SHADER.uniforms[8].name, "uSelected");
    POSTPROCESS_SHADER.uniforms[9].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "uTexel");
    sprintf(POSTPROCESS_SHADER.uniforms[9].name, "uTexel");
    POSTPROCESS_SHADER.uniforms[10].location = glGetUniformLocation(POSTPROCESS_SHADER.program, "cam_right");
    sprintf(POSTPROCESS_SHADER.uniforms[10].name, "cam_right");

}



void init_density_compute_shader()
{

    DENSITY_COMPUTE_SHADER.program = create_compute_shader("./src/mesh/density.comp");

    DENSITY_COMPUTE_SHADER.uniforms[0].location = glGetUniformLocation(DENSITY_COMPUTE_SHADER.program, "uSize");
    sprintf(DENSITY_COMPUTE_SHADER.uniforms[0].name, "uSize");

    DENSITY_COMPUTE_SHADER.uniforms[1].location = glGetUniformLocation(DENSITY_COMPUTE_SHADER.program, "chunk_id");
    sprintf(DENSITY_COMPUTE_SHADER.uniforms[1].name, "chunk_id");

    DENSITY_COMPUTE_SHADER.uniforms[2].location = glGetUniformLocation(DENSITY_COMPUTE_SHADER.program, "chunk_offset");
    sprintf(DENSITY_COMPUTE_SHADER.uniforms[2].name, "chunk_offset");

    DENSITY_COMPUTE_SHADER.uniforms[3].location = glGetUniformLocation(DENSITY_COMPUTE_SHADER.program, "lod");
    sprintf(DENSITY_COMPUTE_SHADER.uniforms[3].name, "lod");
    
    DENSITY_COMPUTE_SHADER.uniforms[4].location = glGetUniformLocation(DENSITY_COMPUTE_SHADER.program, "perlin_tex");
    sprintf(DENSITY_COMPUTE_SHADER.uniforms[4].name, "perlin_tex");
    DENSITY_COMPUTE_SHADER.uniforms[5].location = glGetUniformLocation(DENSITY_COMPUTE_SHADER.program, "fbm_tex");
    sprintf(DENSITY_COMPUTE_SHADER.uniforms[5].name, "fbm_tex");

}

void init_marching_cubes_compute_shader()
{
    MARCHING_CUBES_COMPUTE_SHADER.program = create_compute_shader("./src/mesh/marching_cubes.comp");

    MARCHING_CUBES_COMPUTE_SHADER.uniforms[0].location = glGetUniformLocation(MARCHING_CUBES_COMPUTE_SHADER.program, "uSize");
    sprintf(MARCHING_CUBES_COMPUTE_SHADER.uniforms[0].name, "uSize");
    
    MARCHING_CUBES_COMPUTE_SHADER.uniforms[1].location = glGetUniformLocation(MARCHING_CUBES_COMPUTE_SHADER.program, "chunk_id");
    sprintf(MARCHING_CUBES_COMPUTE_SHADER.uniforms[1].name, "chunk_id");

    MARCHING_CUBES_COMPUTE_SHADER.uniforms[2].location = glGetUniformLocation(MARCHING_CUBES_COMPUTE_SHADER.program, "chunk_offset");
    sprintf(MARCHING_CUBES_COMPUTE_SHADER.uniforms[2].name, "chunk_offset");

    MARCHING_CUBES_COMPUTE_SHADER.uniforms[3].location = glGetUniformLocation(MARCHING_CUBES_COMPUTE_SHADER.program, "perlin_tex");
    sprintf(MARCHING_CUBES_COMPUTE_SHADER.uniforms[3].name, "perlin_tex");
    MARCHING_CUBES_COMPUTE_SHADER.uniforms[4].location = glGetUniformLocation(MARCHING_CUBES_COMPUTE_SHADER.program, "fbm_tex");
    sprintf(MARCHING_CUBES_COMPUTE_SHADER.uniforms[4].name, "fbm_tex");
}

void init_indirect_compute_shader()
{
    INDIRECT_COMPUTE_SHADER.program = create_compute_shader("./src/mesh/indirect.comp");

    INDIRECT_COMPUTE_SHADER.uniforms[0].location = glGetUniformLocation(INDIRECT_COMPUTE_SHADER.program, "chunk_id");
    sprintf(INDIRECT_COMPUTE_SHADER.uniforms[0].name, "chunk_id");
    
    INDIRECT_COMPUTE_SHADER.uniforms[1].location = glGetUniformLocation(INDIRECT_COMPUTE_SHADER.program, "uSize");
    sprintf(INDIRECT_COMPUTE_SHADER.uniforms[1].name, "uSize");

    INDIRECT_COMPUTE_SHADER.uniforms[2].location = glGetUniformLocation(INDIRECT_COMPUTE_SHADER.program, "global_id");
    sprintf(INDIRECT_COMPUTE_SHADER.uniforms[2].name, "global_id");

    INDIRECT_COMPUTE_SHADER.uniforms[3].location = glGetUniformLocation(INDIRECT_COMPUTE_SHADER.program, "new_chunk");
    sprintf(INDIRECT_COMPUTE_SHADER.uniforms[3].name, "new_chunk");

    INDIRECT_COMPUTE_SHADER.uniforms[4].location = glGetUniformLocation(INDIRECT_COMPUTE_SHADER.program, "lod");
    sprintf(INDIRECT_COMPUTE_SHADER.uniforms[4].name, "lod");
}

void init_shaders()
{

    init_sky_shader();

    init_atmosphere_shader();
    init_atmosphere_from_space_shader();
    init_main_shader();
    init_shadow_map_shader();
    init_shadow_water_shader();
    init_mesh_shader();
    init_water_shader();
    init_postprocess_shader();
    init_clouds_shader();

    init_density_compute_shader();
    init_marching_cubes_compute_shader();
    init_indirect_compute_shader();

}
