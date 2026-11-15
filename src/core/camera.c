#include <math.h>
#include <omp.h>
#include "mesh/mesh.h"
#include "core/types.h"
#include "utilities/mathf.h"
#include "core/camera.h"
#include "core/init.h"
#include <stdio.h>


void
    compute_camera_basis
    (
        Camera * cam,
        vec3 world_up
    )

{
    
    cam->forward.x = cosf(cam->yaw) * cosf(cam->pitch);
    cam->forward.y = sinf(cam->yaw) * cosf(cam->pitch);
    cam->forward.z = sinf(cam->pitch);
    cam->forward = vec3_normalize(cam->forward);

    cam->right = vec3_cross(cam->forward, world_up);
    cam->right = vec3_normalize(cam->right);

    cam->up = vec3_cross(cam->right, cam->forward);
    cam->up = vec3_normalize(cam->up);

}

mat4
    make_ortho
    (
        float left,
        float right,
        float bottom,
        float top,
        float near,
        float far
    )

{

    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;
    
    mat4 out;

    out.a0 = 2 / rl;
    out.a1 = 0;
    out.a2 = 0;
    out.a3 = 0;

    out.b0  = 0;
    out.b1  = 2 / tb;
    out.b2  = 0;
    out.b3  = 0;

    out.c0  = 0;
    out.c1  = 0;
    out.c2  = -2 / fn;
    out.c3  = 0;

    out.d0  = -(right + left) / rl;
    out.d1  = -(top + bottom) / tb;
    out.d2  = -(far + near) / fn;
    out.d3  = 1;
    return out;
}


mat4
    make_projection
    (
        float fov_deg,
        float aspect,
        float near,
        float far
    )

{

    float fov_rad = fov_deg * (M_PI / 180.0f);
    float f = 1.0f / tanf(fov_rad / 2.0f);
    mat4 out;
    out.a0 = f / aspect;
    out.a1 = 0;
    out.a2 = 0;
    out.a3 = 0;

    out.b0 = 0;
    out.b1 = f;
    out.b2 = 0;
    out.b3 = 0;

    out.c0 = 0;
    out.c1 = 0;
    out.c2 = (far + near) / (near - far);
    out.c3 = -1;

    out.d0 = 0;
    out.d1 = 0;
    out.d2 = (2 * far * near) / (near - far);
    out.d3 = 0;
    return out;
}

void
    print_mat4
    ( const mat4 *m )

{

    printf("MAT4:\n"
           " % .6f % .6f % .6f % .6f\n"
           " % .6f % .6f % .6f % .6f\n"
           " % .6f % .6f % .6f % .6f\n"
           " % .6f % .6f % .6f % .6f\n",
           m->a0,m->a1,m->a2,m->a3,
           m->b0,m->b1,m->b2,m->b3,
           m->c0,m->c1,m->c2,m->c3,
           m->d0,m->d1,m->d2,m->d3);

}

mat4
    make_look_at
    (
        Camera * cam,
        vec3     eye
    )

{

    vec3 f = cam->forward;
    vec3 s = cam->right;
    vec3 u = cam->up;

    mat4 out;
    out.a0  = s.x;
    out.a1  = u.x;
    out.a2  = -f.x;
    out.a3  = 0;

    out.b0  = s.y;
    out.b1  = u.y;
    out.b2  = -f.y;
    out.b3  = 0;

    out.c0  = s.z;
    out.c1  = u.z;
    out.c2 = -f.z;
    out.c3 = 0;


    out.d0 = -vec3_dot(s, eye);
    out.d1 = -vec3_dot(u, eye);
    out.d2 = vec3_dot(f, eye);
    out.d3 = 1;
    return out;
}


void
    update_light_camera
    ( Camera * lcam )

{

    lcam->cam_data.proj = make_ortho(-50.0f, 50.0f, -50.0f, 50.0f, 1.0f, 200.0f);

    vec3 cam_pos = (vec3){
        cam.cam_data.cam_pos.x,
        cam.cam_data.cam_pos.y,
        cam.cam_data.cam_pos.z
    };

    vec3 offset = (vec3){0, 0, 0};

    vec3 pos = vec3_add(cam_pos, offset);

    vec3 sd = {
        PLANET_DATA.sun_dir.x,
        PLANET_DATA.sun_dir.y,
        PLANET_DATA.sun_dir.z
    };
    
    vec3 negated_sun_dir = {
        -sd.x,
        -sd.y,
        -sd.z
    };
    
    vec3 light_dir = vec3_normalize(negated_sun_dir);

    float distance = 100.0f;
    pos.x += light_dir.x * distance;
    pos.y += light_dir.y * distance;
    pos.z += light_dir.z * distance;

    lcam->cam_data.cam_pos = (vec4){
        pos.x,
        pos.y,
        pos.z,
        0.0
    };

    lcam->pitch = -M_PI / 2;
    lcam->yaw = 0;

    compute_camera_basis(lcam, (vec3){0, 0, 1});

    lcam->target = vec3_add(pos, lcam->forward);
    lcam->cam_data.view = make_look_at(lcam, pos);

}

void
    update_camera
    ( Camera * cam )

{

    cam->cam_data.cam_pos.z += 0.5f;

    compute_camera_basis(cam, (vec3){0, 0, 1});

    cam->cam_data.proj = make_projection(
        FOV,
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
        VIEW_NEAR,
        VIEW_FAR
    );

    cam->target = vec3_add(
        (vec3){
            cam->cam_data.cam_pos.x,
            cam->cam_data.cam_pos.y,
            cam->cam_data.cam_pos.z
        },
        cam->forward
    );

    cam->cam_data.view = make_look_at(
        cam,
        (vec3){
            cam->cam_data.cam_pos.x,
            cam->cam_data.cam_pos.y,
            cam->cam_data.cam_pos.z
        }
    );

    cam->cam_data.inv_view = mat4_inverse(cam->cam_data.view);
    cam->cam_data.inv_proj = mat4_inverse(cam->cam_data.proj);
    cam->cam_data.cam_pos.z -= 0.5f;

}

void
    apply_air_acceleration
    (
        vec2  wish,
        float wish_speed,
        float accel,
        float dt
    )

{

    vec3 fwd = cam.forward;
    float wlen = sqrtf(wish.x * wish.x + wish.y * wish.y);
    float falignment = 0.0f;
    if (wlen > 1e-5f)
    {
        falignment = (fwd.x * wish.x + fwd.y * wish.y) / (wlen);
        falignment = clamp(falignment, -1.0f, 1.0f);
        falignment += 1;
        if (falignment > 1) falignment -= 1;
    }
    wish = (vec2){wish.x + fwd.x, wish.y + fwd.y};
    wlen = sqrtf(wish.x * wish.x + wish.y * wish.y);
    if (wlen > 1e-5f) {
        wish.x /= wlen;
        wish.y /= wlen;
    }

    float vx = cam.cam_data.cam_vel.x;
    float vy = cam.cam_data.cam_vel.y;
    float curr = vx * wish.x + vy * wish.y;
    float add = wish_speed - curr;
    if (add <= 0.0f) return;

    float acc = 0.005 * wish_speed * dt;
    if (acc > add) acc = add;
    if (acc > 0)
    {
        cam.cam_data.cam_vel.x += acc * wish.x * falignment;
        cam.cam_data.cam_vel.y += acc * wish.y * falignment;
    }

    float vlen = sqrtf(vx * vx + vy * vy);
    
    float alignment = 0.0f;
    if (vlen > 1e-5f && wlen > 1e-5f)
    {
        alignment = (vx * wish.x + vy * wish.y) / (vlen * wlen);
        alignment = clamp(alignment, -1.0f, 1.0f);
    }
    float steer = 1.0f - alignment;
    steer *= steer;

    float steer_nudge = 0.005 * steer * dt;
    
    cam.cam_data.cam_vel.x += wish.x * steer_nudge;
    cam.cam_data.cam_vel.y += wish.y * steer_nudge;

    float nlen = sqrtf(cam.cam_data.cam_vel.x * cam.cam_data.cam_vel.x + cam.cam_data.cam_vel.y * cam.cam_data.cam_vel.y);
    if (nlen > vlen)
    {
        float scale = vlen / nlen;
        scale = fminf(scale * 1.1f, 1.0f);
        cam.cam_data.cam_vel.x *= scale;
        cam.cam_data.cam_vel.y *= scale;
    }
    
    
}

void
    apply_planet_velocity
    (
        Camera * cam,
        vec3     move_dir,
        float    dt
    )

{

    static const float MAX_SPEED = 0.5f;
    static const float ACCEL = 0.001f;
    static const float GRAVITY = 0.002f;
    float accel_step = ACCEL * dt;
    float s_len = sqrtf(move_dir.x * move_dir.x + move_dir.y * move_dir.y);
    if (s_len < 0.01f)
    {
        if (cam->on_ground)
        {
            
            cam->cam_data.cam_vel= vec4_scale(cam->cam_data.cam_vel, 0.85f);
        }
        else
        {
            cam->cam_data.cam_vel.z -= GRAVITY * dt;
            
        }
        return;
    }

    vec3 fwd = {cosf(cam->yaw), sinf(cam->yaw), sinf(cam->pitch)};
    vec3 rgt = {-sinf(cam->yaw), cosf(cam->yaw), 0.0f};
    fwd = vec3_normalize(fwd);
    rgt = vec3_normalize(rgt);
    float wish_x = move_dir.x * fwd.x + move_dir.y * rgt.x;
    float wish_y = move_dir.x * fwd.y + move_dir.y * rgt.y;

    float proj_wish = wish_x * rgt.x + wish_y * rgt.y;
    float target_roll = proj_wish * -0.05f;
    if (wish_x == 0.0f && wish_y == 0.0f) target_roll = 0.0f;
    cam->roll = lerp(cam->roll, target_roll, dt * 10.0f);

    if (s_len > 0.0f)
    {
        wish_x /= s_len;
        wish_y /= s_len;
    }
    float cur = cam->cam_data.cam_vel.x * wish_x + cam->cam_data.cam_vel.y * wish_y;
    if (cam->on_ground)
    {
         float add = MAX_SPEED - cur;
         if (add)
         {
             float push = fminf(add, accel_step);
             cam->cam_data.cam_vel.x += wish_x * push;
             cam->cam_data.cam_vel.y += wish_y * push;
         }
    }
    else
    {
        apply_air_acceleration((vec2){wish_x, wish_y}, s_len, 18, dt);
        cam->cam_data.cam_vel.z -= GRAVITY * dt;
    }


    float nlen = sqrtf(cam->cam_data.cam_vel.x * cam->cam_data.cam_vel.x + cam->cam_data.cam_vel.y * cam->cam_data.cam_vel.y);
    if (nlen > MAX_SPEED)
    {
        float scale = MAX_SPEED / nlen;
        cam->cam_data.cam_vel.x *= scale;
        cam->cam_data.cam_vel.y *= scale;
    }


}

vec3
    find_density_normal
    (
        vec3  sample,
        float d
    )

{
    
    vec3 normal;
    normal.x = calc_density(vec3_add(sample, (vec3){d, 0, 0})) - calc_density(vec3_add(sample, (vec3){-d, 0, 0}));
    normal.y = calc_density(vec3_add(sample, (vec3){0, d, 0})) - calc_density(vec3_add(sample, (vec3){0, -d, 0}));
    normal.z = calc_density(vec3_add(sample, (vec3){0, 0, d})) - calc_density(vec3_add(sample, (vec3){0, 0, -d}));
    normal = vec3_normalize(normal);

    return normal;
    
}

bool
    find_surface_entry
    (
        vec3   start,
        vec3   p,
        vec3 * entry,
        vec3 * normal,
        float  voxel_size,
        float  radius,
        float  planet_radius
    )

{

    const int STEP_AMT = 100;
    const float THRESHOLD = 0.2f;

    // adjust for planet radius (we are technically higher than surface level)
    start.z -= planet_radius;
    p.z -= planet_radius;

    float d = calc_density(start);
    vec3 dir = vec3_sub(p, start);
    float step_len = vec3_length(vec3_scale(dir, (float)1 / STEP_AMT));
    vec3 dir_step = vec3_scale(vec3_normalize(dir), step_len);
    float prev_d = d;

    // if we start within a surface, we push out
    bool started_inside = (prev_d < THRESHOLD);
    if (started_inside)
    {
        
        *entry = start;
        *normal = find_density_normal(*entry, voxel_size);
        return true;
    }
    
    // iterate to STEP AMT to find a surface in the direction of movement
    vec3 prev = start;
    for (int i = 0; i < STEP_AMT; i++)
    {
        
        vec3 o = vec3_add(prev, dir_step);
        d = calc_density(o);

        if ((d - THRESHOLD) * (prev_d - THRESHOLD) < 0.0f)
        {
            
            // interpolate the entry density and push out to find the entry point
            float t = prev_d / (prev_d - d);
            *entry = vec3_add(prev, vec3_scale(vec3_sub(o, prev), t));
            *normal = find_density_normal(*entry, voxel_size);
            return true;

        }

        prev = o;
        prev_d = d;

    }
    return false;

}

float
    find_surface_z
    ( vec3 p )

{

    float z0 = p.z; 
    float z1 = p.z - 1.0f; 

    float d0 = calc_density((vec3){p.x, p.y, z0});
    float d1 = calc_density((vec3){p.x, p.y, z1});

    if (d0 * d1 > 0.0f) {
        return p.z;
    }

    for (int i = 0; i < 100; i++)
    {
        float zm = 0.5f * (z0 + z1);
        float dm = calc_density((vec3){p.x, p.y, zm});

        if (dm > 0.0f)
            z0 = zm;
        else
            z1 = zm;
    }

    return 0.5f * (z0 + z1);
}

float
    find_surface
    (
        vec3  cam_pos,
        vec3  p,
        float planet_radius
    )

{
    
    const int FAIL = 5000;
    const float STEP_SIZE = 0.0065f;
    const float THRESHOLD = 0.35f;

    vec3 prop = p;
    prop.z = cam_pos.z + 0.5f - planet_radius;
    float fail_bottom = calc_density((vec3){p.x, p.y, prop.z - STEP_SIZE * FAIL});
    if (fail_bottom > THRESHOLD)
    {
        return -9999;
    }
    float top = p.z;
    float density = calc_density(prop);

    bool top_outside = false;
    if (density > THRESHOLD)
    {
        int failsafe = 0;
        bool valid_hit = false;
        while (density > THRESHOLD && failsafe < FAIL)
        {
            density = calc_density(prop);
            if (density <= THRESHOLD)
            {

                valid_hit = true;
                break;
            }
            prop.z -= STEP_SIZE;
            failsafe++;
        }
        if (valid_hit && failsafe < FAIL)
        {
            top_outside = true;
            top = prop.z;
        }
    }
    
    if (!top_outside)
    {
        top = -9999;
    }
    return top;
}

bool was_on_ground = false;
bool was_not_on_ground = false;
bool ground_hit = false;

const float radius = 0.5;
bool hit_ground(float z, float current_floor)
{
    cam.cam_data.cam_pos.z = current_floor;
    float dist_to_floor = z - (current_floor);
    if (dist_to_floor <= 0.1f)
    {
        
        cam.cam_data.cam_vel.z = 0;
        cam.on_ground = true;

        ground_hit = true;
        return true;
    }
    else
    {
        if (!was_on_ground) was_on_ground = true;
        cam.on_ground = false;
        return false;
    }
    
}




bool
    check_collision
    (
        vec3   start, // starting position
        vec3   final, // proposed position
        vec3 * normal, // pointer to hit surface normal
        vec3 * entrance, // pointer to hit position
        float  d, // voxel size
        float  current_floor, // current ground
        float  planet_radius
    )

{

    // check sampling for current height of the player to see if we are too high or too low
    was_on_ground = cam.on_ground;
    ground_hit = hit_ground(final.z - planet_radius, current_floor);
    
    if (!was_on_ground && ground_hit) { final.z = current_floor + 0.005f; }

    bool entered = find_surface_entry(start, final, entrance, normal, d, radius, planet_radius);
    if (entered){
        
        return true;

    }
    return false;

}


void
    resolve_collisions
    (
        Camera * cam,
        float    planet_radius
    )

{

    ground_hit = false;


    float chunk_world_size = CHUNK_WORLD_SIZE; 

    float voxel_size = chunk_world_size / 64.0;


    vec3 cam_pos = (vec3){cam->cam_data.cam_pos.x, cam->cam_data.cam_pos.y, cam->cam_data.cam_pos.z};
    vec3 cam_vel = (vec3){cam->cam_data.cam_vel.x, cam->cam_data.cam_vel.y, cam->cam_data.cam_vel.z};

    vec3 proposed = vec3_add(cam_pos, cam_vel);
    
    vec3 prop2 = proposed;

    prop2.z = 0;
    
    float d = voxel_size;
    Timer find_s, cc;
    start_timer(&find_s);
    float ground = find_surface(cam_pos, proposed, planet_radius);
    
    vec3 collision_normal = (vec3){99, 99, 99};
    vec3 entrance;

    start_timer(&cc);
    if (!check_collision(
            (vec3){cam_pos.x, cam_pos.y, cam_pos.z},
            (vec3){proposed.x, proposed.y, proposed.z},
            &collision_normal,
            &entrance,
            d,
            ground,
            planet_radius
        ))
    {
        cam->cam_data.cam_pos.x = proposed.x;
        cam->cam_data.cam_pos.y = proposed.y;
        cam->cam_data.cam_pos.z = proposed.z;

    }
    else
    {
        if (collision_normal.x != 99 && collision_normal.y != 99 && collision_normal.z != 99)
        {
            vec3 wish = vec3_sub(proposed, cam_pos);

            float vel_dot_n = vec3_dot(wish, collision_normal);

            if (vel_dot_n < 0.0f)
            {
                wish = vec3_sub(wish, vec3_scale(collision_normal, vel_dot_n));
                proposed.x = cam_pos.x + wish.x;
                proposed.y = cam_pos.y + wish.y;
                proposed.z = cam_pos.z + wish.z;

                proposed.x += collision_normal.x * 0.01f;
                proposed.y += collision_normal.y * 0.01f;
                proposed.z += collision_normal.z * 0.01f;
            }
            
            cam->cam_data.cam_vel.x = wish.x;
            cam->cam_data.cam_vel.y = wish.y;
            cam->cam_data.cam_vel.z = wish.z;

            cam->cam_data.cam_pos.x = proposed.x;
            cam->cam_data.cam_pos.y = proposed.y;
            cam->cam_data.cam_pos.z = proposed.z;
        
        }
        else
        {
            cam->cam_data.cam_vel.x = 0;
            cam->cam_data.cam_vel.y = 0;
            cam->cam_data.cam_vel.z = 0;
            
        }
        return;
    }
    
}



void
    apply_velocity
    (
        Camera * cam,
        vec3     move_dir,
        float    dt
    )

{

    static const float MAX_SPEED = 100.0f;
    static const float ACCEL = 1.0f;
    float accel_step = ACCEL * dt;
    float s_len = sqrtf(move_dir.x * move_dir.x + move_dir.y * move_dir.y);
    if (s_len < 0.01f)
    {
        cam->cam_data.cam_vel= vec4_scale(cam->cam_data.cam_vel, 0.85f);
    }

    vec3 fwd = {cosf(cam->yaw), sinf(cam->yaw), sinf(cam->pitch)};
    vec3 rgt = {-sinf(cam->yaw), cosf(cam->yaw), 0.0f};
    fwd = vec3_normalize(fwd);
    rgt = vec3_normalize(rgt);
    float wish_x = move_dir.x * fwd.x + move_dir.y * rgt.x;
    float wish_y = move_dir.x * fwd.y + move_dir.y * rgt.y;
    float wish_z = move_dir.x * fwd.z + move_dir.y * rgt.z;

    float proj_wish = wish_x * rgt.x + wish_y * rgt.y + wish_z * rgt.z;
    float target_roll = proj_wish * -0.05f;
    if (wish_x == 0.0f && wish_y == 0.0f) target_roll = 0.0f;
    cam->roll = lerp(cam->roll, target_roll, dt * 10.0f);

    if (s_len > 0.0f)
    {
        wish_x /= s_len;
        wish_y /= s_len;
        wish_z /= s_len;
    }
    float cur = cam->cam_data.cam_vel.x * wish_x + cam->cam_data.cam_vel.y * wish_y + cam->cam_data.cam_vel.z * wish_z;
    float add = MAX_SPEED - cur;
    if (add)
    {
        float push = fminf(add, accel_step);
        cam->cam_data.cam_vel.x += wish_x * push;
        cam->cam_data.cam_vel.y += wish_y * push;
        cam->cam_data.cam_vel.z += wish_z * push;
    }
    cam->cam_data.cam_pos.x += cam->cam_data.cam_vel.x;
    cam->cam_data.cam_pos.y += cam->cam_data.cam_vel.y;
    cam->cam_data.cam_pos.z += cam->cam_data.cam_vel.z;
}

float
    signed_angle
    (
        vec2 a,
        vec2 b
    )

{

    float cross = a.x*b.y - a.y*b.x;
    float dot = a.x*b.x + a.y*b.y;
    return atan2f(cross, dot);

}

float
    signed_angle_3d
    (
        vec3 a,
        vec3 b,
        vec3 n
    )

{

   vec3 cross = vec3_cross(a, b); 
   float dot = vec3_dot(cross, n);
   float angle = atan2f(dot, vec3_dot(a, b));
   if (dot < 0) angle = -angle;
   return angle;

}

float
    pitch_difference
    (
        vec3 a,
        vec3 b
    )

{

    float pa = atan2f(a.z, sqrtf(a.x * a.x + a.y * a.y));
    float pb = atan2f(b.z, sqrtf(b.x * b.x + b.y * b.y));
    return pb - pa;
    
}


void
    look_at_selected_body ()
    
{
    
    vec4 pos;
    
    glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 60);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, PARTICLE_SSBO);
    vec4 * ptr = glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER,
        offsetof(ParticleData, p) + sizeof(Particle) * GLOBALS.selected_body,
        sizeof(vec4),
        GL_MAP_READ_BIT
    );

    memcpy(&pos, ptr, sizeof(vec4));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    vec3 ppos = {pos.x, pos.y, pos.z};

    vec3 fdest = vec3_normalize(vec3_sub(ppos, (vec3){cam.cam_data.cam_pos.x, cam.cam_data.cam_pos.y, cam.cam_data.cam_pos.z}));
    
    vec2 forward2 = { cam.forward.x, cam.forward.y };

    float s_angle = signed_angle(forward2, (vec2){fdest.x, fdest.y});
    cam.yaw += s_angle;


    cam.forward.x = cosf(cam.yaw) * cosf(cam.pitch);
    cam.forward.y = sinf(cam.yaw) * cosf(cam.pitch);
    cam.forward.z = sinf(cam.pitch);
    cam.forward = vec3_normalize(cam.forward);
    

    cam.pitch += pitch_difference(cam.forward, fdest);

    vec3 final_pos = vec3_sub(ppos, vec3_scale(fdest, 10.0f));
    cam.cam_data.cam_pos.x = final_pos.x;
    cam.cam_data.cam_pos.y = final_pos.y;
    cam.cam_data.cam_pos.z = final_pos.z;
    
}

