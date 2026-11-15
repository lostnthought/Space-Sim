#ifndef CAMERA_H
#define CAMERA_H

#include "core/types.h"
extern Camera cam;

void
    print_mat4
    ( const mat4 *m );

mat4
    make_ortho
    (
        float left,
        float right,
        float bottom,
        float top,
        float near,
        float far
    );

mat4
    make_projection
    (
        float fov_deg,
        float aspect,
        float near,
        float far
    );

void
    update_camera
    ( Camera * cam );
    
void
    resolve_collisions
    (
        Camera * current_cam,
        float planet_radius
    );

void
    apply_velocity
    (
        Camera * current_cam,
        vec3 move_dir,
        float dt
    );

mat4
    make_look_at
    (
        Camera * cam,
        vec3 eye
    );

void
    apply_planet_velocity
    (
        Camera * current_cam,
        vec3 move_dir,
        float dt
    );

void
    compute_camera_basis
    (
        Camera * cam,
        vec3 world_up
    );

void
    update_light_camera
    ( Camera * lcam );
    
void
    look_at_selected_body ();


#endif
