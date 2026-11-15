
#include "mesh/mesh.h"
#include "core/types.h"
#include "core/init.h"

bool s = false;
bool change = false;
// 32 bits
void
    upload_param_to_ssbo
    (
        size_t offset,
        void * val
    )

{
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, PLANET_DATA_SSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(float), val);
    change = true;

}

// Main UI, uploads to an ssbo with params on update using dirty flag.
void
    display_main_menu ()

{

    change = false;
    const float OFFSET_BOUNDS = 100.0f;
    const float FREQ_BOUNDS = 100.0f;
    const float AMP_BOUNDS = 100.0f;

    if
    (
        nk_begin
        (
            NK_CTX,
            "Main",
            nk_rect(20, 20, 300, SCREEN_HEIGHT),
            NK_WINDOW_TITLE | NK_WINDOW_SCROLL_AUTO_HIDE
        )
    )
    {

        struct nk_rect win_bounds = nk_window_get_bounds(NK_CTX);
        if (nk_input_is_mouse_hovering_rect(&NK_CTX->input, win_bounds)) { NUKLEAR_HOVERED = true; }

        size_t offset = offsetof(PlanetData, sun_dir);


        nk_layout_row_dynamic(NK_CTX, 20, 1);
        float min_m = nk_propertyf(NK_CTX, "Min Mass", 0.0f, GLOBALS.min_mass, 1000000.0f, 0.01f, 0.01f);
        if (min_m != GLOBALS.min_mass)
        {            
            GLOBALS.min_mass = min_m;
        }
        float max_m = nk_propertyf(NK_CTX, "Max Mass", 0.0f, GLOBALS.min_mass, 1000000.0f, 0.01f, 0.01f);
        if (max_m != GLOBALS.max_mass)
        {            
            GLOBALS.max_mass = max_m;
        }
        float mass_pow = nk_propertyf(NK_CTX, "Mass Pow", 0.0f, GLOBALS.mass_pow, 1000000, 0.01f, 0.01f);
        if (mass_pow != GLOBALS.mass_pow)
        {            
            GLOBALS.mass_pow = mass_pow;
        }
        float radius= nk_propertyf(NK_CTX, "Radius", 0.0f, GLOBALS.radius, 1000000, 0.01f, 0.01f);
        if (radius != GLOBALS.radius)
        {            
            GLOBALS.radius = radius;
        }
        nk_bool planet_toggle = GLOBALS.planet_meshing_toggle;
        if (nk_checkbox_label(NK_CTX, "Planet Toggle", &planet_toggle))
        {
            GLOBALS.planet_meshing_toggle = planet_toggle;
            if (GLOBALS.planet_meshing_toggle) init_chunk_data();
        }
        nk_bool water_toggle = GLOBALS.water_toggle;
        if (nk_checkbox_label(NK_CTX, "Water Toggle", &water_toggle))
        {
            GLOBALS.water_toggle = water_toggle;
        }
        nk_bool particle_toggle = GLOBALS.particle_toggle;
        if (nk_checkbox_label(NK_CTX, "Particle Toggle", &particle_toggle))
        {
            GLOBALS.particle_toggle = particle_toggle;
        }
        nk_bool sky_toggle = GLOBALS.sky_toggle;
        if (nk_checkbox_label(NK_CTX, "Sky Toggle", &sky_toggle))
        {
            GLOBALS.sky_toggle = sky_toggle;
        }
        nk_bool profiling = GLOBALS.profiling;
        if (nk_checkbox_label(NK_CTX, "Profiling", &profiling))
        {
            GLOBALS.profiling = profiling;
        }
        if (nk_button_label(NK_CTX, "Reset Sim"))
        {
            init_random();
        }
        char origin[256];
        sprintf(origin, "Origin: %f %f %f", planet_origin.x, planet_origin.y, planet_origin.z);

        nk_label(NK_CTX, origin, NK_TEXT_ALIGN_LEFT);
        if (nk_button_label(NK_CTX, "Reset Origin"))
        {
            planet_origin = (vec3){0,0,0};
        }

        
        nk_layout_row_dynamic(NK_CTX, 20, 1);
        nk_label(NK_CTX, "Sun Dir:", NK_TEXT_ALIGN_LEFT);

        nk_layout_row_dynamic(NK_CTX, 20, 3);
        nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "X: %f", PLANET_DATA.sun_dir.x);
        nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Y: %f", PLANET_DATA.sun_dir.y);
        nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Z: %f", PLANET_DATA.sun_dir.z);
        float x0 = nk_propertyf(NK_CTX, "Sun X", -1.0f, PLANET_DATA.sun_dir.x, 1, 0.001f, 0.001f);
        if (x0 != PLANET_DATA.sun_dir.x)
        {

            PLANET_DATA.sun_dir.x = x0;
            upload_param_to_ssbo(offset, &x0);
        }
        offset += sizeof(float);
        float y0 = nk_propertyf(NK_CTX, "Sun Y", -1.0f, PLANET_DATA.sun_dir.y, 1, 0.001f, 0.001f);
        if (y0 != PLANET_DATA.sun_dir.y)
        {

            PLANET_DATA.sun_dir.y = y0;
            upload_param_to_ssbo(offset, &y0);
        }
        offset += sizeof(float);
        float z0 = nk_propertyf(NK_CTX, "Sun Z", -1.0f, PLANET_DATA.sun_dir.z, 1, 0.001f, 0.001f);
        if (z0 != PLANET_DATA.sun_dir.z)
        {

            PLANET_DATA.sun_dir.z = z0;
            upload_param_to_ssbo(offset, &z0);
        }
        offset += sizeof(float);

        nk_layout_row_dynamic(NK_CTX, 20, 1);
        nk_label(NK_CTX, "Sun Color:", NK_TEXT_ALIGN_LEFT);

        offset = offsetof(PlanetData, sun_color);
        struct nk_colorf sun_color= {
            PLANET_DATA.sun_color.x,
            PLANET_DATA.sun_color.y,
            PLANET_DATA.sun_color.z,
            1.0f
        };

        nk_label(NK_CTX, "Sun Color", NK_TEXT_LEFT);

        if (nk_combo_begin_color(NK_CTX, nk_rgb_cf(sun_color), nk_vec2(150, 150))) {
            nk_layout_row_dynamic(NK_CTX, 120, 1);
            sun_color = nk_color_picker(NK_CTX, sun_color, NK_RGB);

            nk_layout_row_dynamic(NK_CTX, 25, 1);
            sun_color.r = nk_propertyf(NK_CTX, "#R:", 0.0f, sun_color.r, 1.0f, 0.01f, 0.005f);
            sun_color.g = nk_propertyf(NK_CTX, "#G:", 0.0f, sun_color.g, 1.0f, 0.01f, 0.005f);
            sun_color.b = nk_propertyf(NK_CTX, "#B:", 0.0f, sun_color.b, 1.0f, 0.01f, 0.005f);

            if (sun_color.r != PLANET_DATA.sun_color.x)
            {
                PLANET_DATA.sun_color.x = sun_color.r;
                upload_param_to_ssbo(offset, &sun_color.r);
            }
            offset += sizeof(float);
            if (sun_color.g != PLANET_DATA.sun_color.y)
            {
                PLANET_DATA.sun_color.y = sun_color.g;
                upload_param_to_ssbo(offset, &sun_color.g);
            }
            offset += sizeof(float);
            if (sun_color.b != PLANET_DATA.sun_color.z)
            {
                PLANET_DATA.sun_color.z = sun_color.b;
                upload_param_to_ssbo(offset, &sun_color.b);
            }
            offset += sizeof(float);
            nk_combo_end(NK_CTX);
        }

        nk_layout_row_dynamic(NK_CTX, 20, 1);
        float esun = nk_propertyf(NK_CTX, "ESun", 0, ESun, 50, 0.01f, 0.01f);
        ESun = esun;

        struct nk_colorf wavelength_color= {
            wavelength.x,
            wavelength.y,
            wavelength.z,
            1.0f
        };

        nk_label(NK_CTX, "Wavelength Color", NK_TEXT_LEFT);

        if (nk_combo_begin_color(NK_CTX, nk_rgb_cf(wavelength_color), nk_vec2(150, 150))) {
            nk_layout_row_dynamic(NK_CTX, 120, 1);
            wavelength_color = nk_color_picker(NK_CTX, wavelength_color, NK_RGB);

            nk_layout_row_dynamic(NK_CTX, 25, 1);
            wavelength_color.r = nk_propertyf(NK_CTX, "#R:", 0.0f, wavelength_color.r, 1.0f, 0.01f, 0.005f);
            wavelength_color.g = nk_propertyf(NK_CTX, "#G:", 0.0f, wavelength_color.g, 1.0f, 0.01f, 0.005f);
            wavelength_color.b = nk_propertyf(NK_CTX, "#B:", 0.0f, wavelength_color.b, 1.0f, 0.01f, 0.005f);

            wavelength.x = wavelength_color.r;
            wavelength.y = wavelength_color.g;
            wavelength.z = wavelength_color.b;
            nk_combo_end(NK_CTX);
        }

        
        nk_label(NK_CTX, "Cam Pos World:", NK_TEXT_ALIGN_LEFT);

        nk_layout_row_dynamic(NK_CTX, 20, 3);
        nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "X: %f", cam_pos_world.x);
        nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Y: %f", cam_pos_world.y);
        nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Z: %f", cam_pos_world.z);
        float cx = nk_propertyf(NK_CTX, "Cam X", -1.0f, cam_pos_world.x, 2, 0.001f, 0.001f);
        cam_pos_world.x = cx;
        float cy = nk_propertyf(NK_CTX, "Cam Y", -1.0f, cam_pos_world.y, 2, 0.001f, 0.001f);
        cam_pos_world.y = cy;
        float cz = nk_propertyf(NK_CTX, "Cam Z", -1.0f, cam_pos_world.z, 2, 0.001f, 0.001f);
        cam_pos_world.z = cz;


        nk_layout_row_dynamic(NK_CTX, 20, 1);
        float g= nk_propertyf(NK_CTX, "G", -1.0f, G, 2, 0.01f, 0.01f);
        G = g;
        float fr = nk_propertyd(NK_CTX, "fKr", 0, fKr, 1, 0.0001f, 0.0001f);
        fKr = fr;
        float fm = nk_propertyd(NK_CTX, "fKm", 0, fKm, 1, 0.0001f, 0.0001f);
        fKm = fm;
        
        nk_layout_row_dynamic(NK_CTX, 20, 1);

        
        offset = 0;
        int warp_count = nk_propertyi(NK_CTX, "Warp Count", 0, PLANET_DATA.warp_count, MAX_LAYERS, 1, 0.1f);
        if (warp_count != PLANET_DATA.warp_count)
        {
            PLANET_DATA.warp_count = warp_count;
            upload_param_to_ssbo(offsetof(PlanetData, warp_count), &warp_count);
        }
        offset = offsetof(PlanetData, warp_layers);

        for (int i = 0; i < PLANET_DATA.warp_count; i++)
        {

            nk_layout_row_dynamic(NK_CTX, 20, 1);
            char l[128];
            sprintf(l, "Warp %d:", i);
            nk_label(NK_CTX, l, NK_TEXT_ALIGN_LEFT);
            nk_label(NK_CTX, "Offset 0:", NK_TEXT_ALIGN_LEFT);
            
            nk_layout_row_dynamic(NK_CTX, 20, 3);

            char xs[128];
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "X0 %f", PLANET_DATA.warp_layers[i].offset0.x);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Y0 %f", PLANET_DATA.warp_layers[i].offset0.y);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Z0 %f", PLANET_DATA.warp_layers[i].offset0.z);
            sprintf(xs, "%d X0", i);
            float x0 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset0.x, OFFSET_BOUNDS, 1, 1);
            if (x0 != PLANET_DATA.warp_layers[i].offset0.x)
            {

                PLANET_DATA.warp_layers[i].offset0.x = x0;
                upload_param_to_ssbo(offset, &x0);
            }
            offset += sizeof(float);
            sprintf(xs, "%d Y0", i);
            float y0 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset0.y, OFFSET_BOUNDS, 1, 1);
            if (y0 != PLANET_DATA.warp_layers[i].offset0.y)
            {

                PLANET_DATA.warp_layers[i].offset0.y = y0;
                upload_param_to_ssbo(offset, &y0);
            }
            offset += sizeof(float);
            sprintf(xs, "%d Z0", i);
            float z0 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset0.z, OFFSET_BOUNDS, 1, 1);
            if (z0 != PLANET_DATA.warp_layers[i].offset0.z)
            {

                PLANET_DATA.warp_layers[i].offset0.z = z0;
                upload_param_to_ssbo(offset, &z0);
            }
            offset += sizeof(float);


            nk_layout_row_dynamic(NK_CTX, 20, 2);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Freq0: %f", PLANET_DATA.warp_layers[i].freq0);
            sprintf(xs, "%d Freq0", i);
            float freq0 = nk_propertyf(NK_CTX, xs, -FREQ_BOUNDS, PLANET_DATA.warp_layers[i].freq0, FREQ_BOUNDS, 0.0001f, 0.0001f);
            if (freq0 != PLANET_DATA.warp_layers[i].freq0)
            {
                PLANET_DATA.warp_layers[i].freq0 = freq0;
                upload_param_to_ssbo(offset, &freq0);
                
            }
            offset += sizeof(float);

            nk_layout_row_dynamic(NK_CTX, 20, 3);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "X1 %f", PLANET_DATA.warp_layers[i].offset1.x);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Y1 %f", PLANET_DATA.warp_layers[i].offset1.y);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Z1 %f", PLANET_DATA.warp_layers[i].offset1.z);

            sprintf(xs, "%d X1", i);
            float x1 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset1.x, OFFSET_BOUNDS, 1, 1);
            if (x1 != PLANET_DATA.warp_layers[i].offset1.x)
            {

                PLANET_DATA.warp_layers[i].offset1.x = x1;
                upload_param_to_ssbo(offset, &x1);
            }
            offset += sizeof(float);
            sprintf(xs, "%d Y1", i);
            float y1 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset1.y, OFFSET_BOUNDS, 1, 1);
            if (y1 != PLANET_DATA.warp_layers[i].offset1.y)
            {

                PLANET_DATA.warp_layers[i].offset1.y = y1;
                upload_param_to_ssbo(offset, &y1);
            }
            offset += sizeof(float);
            sprintf(xs, "%d Z1", i);
            float z1 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset1.z, OFFSET_BOUNDS, 1, 1);
            if (z1 != PLANET_DATA.warp_layers[i].offset1.z)
            {

                PLANET_DATA.warp_layers[i].offset1.z = z1;
                upload_param_to_ssbo(offset, &z1);
            }
            offset += sizeof(float);


            nk_layout_row_dynamic(NK_CTX, 20, 2);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Freq1: %f", PLANET_DATA.warp_layers[i].freq1);
            sprintf(xs, "%d Freq1", i);
            float freq1 = nk_propertyf(NK_CTX, xs, -FREQ_BOUNDS, PLANET_DATA.warp_layers[i].freq1, FREQ_BOUNDS, 0.0001f, 0.0001f);
            if (freq1 != PLANET_DATA.warp_layers[i].freq1)
            {
                PLANET_DATA.warp_layers[i].freq1 = freq1;
                upload_param_to_ssbo(offset, &freq1);
                
            }
            offset += sizeof(float);
            nk_layout_row_dynamic(NK_CTX, 20, 3);

            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "X2 %f", PLANET_DATA.warp_layers[i].offset2.x);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Y2 %f", PLANET_DATA.warp_layers[i].offset2.y);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Z2 %f", PLANET_DATA.warp_layers[i].offset2.z);
            sprintf(xs, "%d X2", i);
            float x2 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset2.x, OFFSET_BOUNDS, 1, 1);
            if (x2 != PLANET_DATA.warp_layers[i].offset2.x)
            {

                PLANET_DATA.warp_layers[i].offset2.x = x2;
                upload_param_to_ssbo(offset, &x2);
            }
            offset += sizeof(float);
            sprintf(xs, "%d Y2", i);
            float y2 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset2.y, OFFSET_BOUNDS, 1, 1);
            if (y2 != PLANET_DATA.warp_layers[i].offset2.y)
            {

                PLANET_DATA.warp_layers[i].offset2.y = y2;
                upload_param_to_ssbo(offset, &y2);
            }
            offset += sizeof(float);
            sprintf(xs, "%d Z2", i);
            float z2 = nk_propertyf(NK_CTX, xs, -OFFSET_BOUNDS, PLANET_DATA.warp_layers[i].offset2.z, OFFSET_BOUNDS, 1, 1);
            if (z2 != PLANET_DATA.warp_layers[i].offset2.z)
            {

                PLANET_DATA.warp_layers[i].offset2.z = z2;
                upload_param_to_ssbo(offset, &z2);
            }
            offset += sizeof(float);


            nk_layout_row_dynamic(NK_CTX, 20, 2);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Freq2: %f", PLANET_DATA.warp_layers[i].freq2);
            sprintf(xs, "%d Freq2", i);
            float freq2 = nk_propertyf(NK_CTX, xs, -FREQ_BOUNDS, PLANET_DATA.warp_layers[i].freq2, FREQ_BOUNDS, 0.0001f, 0.0001f);
            if (freq2 != PLANET_DATA.warp_layers[i].freq2)
            {
                PLANET_DATA.warp_layers[i].freq2 = freq2;
                upload_param_to_ssbo(offset, &freq2);
                
            }
            offset += sizeof(float);

            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Amp: %f", PLANET_DATA.warp_layers[i].amp);
            sprintf(xs, "%d Amp", i);
            float amp = nk_propertyf(NK_CTX, xs, -AMP_BOUNDS, PLANET_DATA.warp_layers[i].amp, AMP_BOUNDS, 0.0001f, 0.0001f);

            if (amp != PLANET_DATA.warp_layers[i].amp)
            {
                PLANET_DATA.warp_layers[i].amp = amp;
                upload_param_to_ssbo(offset, &amp);
                
            }
            offset += sizeof(float);
            offset += sizeof(float);
            offset += sizeof(float);
            offset += sizeof(float);
        }

        offset = sizeof(int);
        nk_layout_row_dynamic(NK_CTX, 20, 1);
        int perlin_count = nk_propertyi(NK_CTX, "Perlin Count", 0, PLANET_DATA.perlin_count, MAX_LAYERS, 1, 0.1f);
        if (perlin_count!= PLANET_DATA.perlin_count)
        {
            PLANET_DATA.perlin_count = perlin_count;
            upload_param_to_ssbo(offsetof(PlanetData, perlin_count), &perlin_count);
        }

        offset = offsetof(PlanetData, perlin_layers);

        for (int i = 0; i < PLANET_DATA.perlin_count; i++)
        {
            
            char l[128];
            nk_layout_row_dynamic(NK_CTX, 20, 1);
            sprintf(l, "Perlin %d:", i);
            nk_label(NK_CTX, l, NK_TEXT_ALIGN_LEFT);

            char f[128];
            nk_layout_row_dynamic(NK_CTX, 20, 2);

            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Freq: %f", PLANET_DATA.perlin_layers[i].freq);
            sprintf(f, "%d PFreq", i);
            float freq = nk_propertyd(NK_CTX, f, -FREQ_BOUNDS, PLANET_DATA.perlin_layers[i].freq, FREQ_BOUNDS, 0.0001f, 0.0001f);

            if (freq != PLANET_DATA.perlin_layers[i].freq)
            {
                PLANET_DATA.perlin_layers[i].freq = freq;
                upload_param_to_ssbo(offset, &freq);
            }

            offset += sizeof(float);
            char a[128];
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Amp: %f", PLANET_DATA.perlin_layers[i].amp);
            sprintf(a, "%d PAmp", i);
            float amp = nk_propertyd(NK_CTX, a, -AMP_BOUNDS, PLANET_DATA.perlin_layers[i].amp, AMP_BOUNDS, 0.0001f, 0.0001f);
            if (amp != PLANET_DATA.perlin_layers[i].amp)
            {
                PLANET_DATA.perlin_layers[i].amp = amp;
                upload_param_to_ssbo(offset, &amp);
            }

            offset += sizeof(float);
            offset += sizeof(float);
            offset += sizeof(float);
        }
        

        nk_layout_row_dynamic(NK_CTX, 20, 1);
        int fbm_count = nk_propertyi(NK_CTX, "FBM Count", 0, PLANET_DATA.fbm_count, MAX_LAYERS, 1, 0.1f);
        if (fbm_count != PLANET_DATA.fbm_count)
        {
            PLANET_DATA.fbm_count = fbm_count;
            upload_param_to_ssbo(offsetof(PlanetData, fbm_count), &fbm_count);
            
        }
        offset = offsetof(PlanetData, fbm_layers);

        for (int i = 0; i < PLANET_DATA.fbm_count; i++)
        {
            
            char l[128];
            sprintf(l, "FBM %d:", i);
            nk_layout_row_dynamic(NK_CTX, 20, 1);
            nk_label(NK_CTX, l, NK_TEXT_ALIGN_LEFT);

            char f[128];
            nk_layout_row_dynamic(NK_CTX, 20, 2);
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Freq: %f", PLANET_DATA.fbm_layers[i].freq);
            sprintf(f, "%d FFreq", i);
            float freq = nk_propertyd(NK_CTX, f, -FREQ_BOUNDS, PLANET_DATA.fbm_layers[i].freq, FREQ_BOUNDS, 0.0001f, 0.0001f);
            if (freq != PLANET_DATA.fbm_layers[i].freq)
            {
                
                PLANET_DATA.fbm_layers[i].freq = freq;
                upload_param_to_ssbo(offset, &freq);
            }

            offset += sizeof(float);

            char a[128];
            nk_labelf(NK_CTX, NK_TEXT_ALIGN_LEFT, "Amp: %f", PLANET_DATA.fbm_layers[i].amp);
            sprintf(a, "%d FAmp", i);
            float amp = nk_propertyd(NK_CTX, a, -AMP_BOUNDS, PLANET_DATA.fbm_layers[i].amp, AMP_BOUNDS, 0.0001f, 0.0001f);

            if (amp != PLANET_DATA.fbm_layers[i].amp)
            {
                
                PLANET_DATA.fbm_layers[i].amp = amp;
                upload_param_to_ssbo(offset, &amp);
            }

            offset += sizeof(float);
            offset += sizeof(float);
            offset += sizeof(float);
        }


    }

    nk_end(NK_CTX);

    // free chunks and rebuild
    if (change)
    {
        for (int l = 0; l < LODMAX; l++)
        {
            for (int i = 0; i < MAX_CHUNKS_LOD[l]; i++)
            {
                free_parent(l, i);
                free_child(l, i);
                free_current_parent(l, i);
            }
        }
        
        dirty_cubemap = true;
    }
    
}
