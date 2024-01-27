#include "pe_core.h"
#include "client/pe_platform.h"
#include "client/pe_graphics.h"
#include "client/pe_input.h"
#include "client/pe_model.h"
#include "pe_time.h"
#include "pe_profile.h"

#include <stdio.h>

#include "HandmadeMath.h"

int main(int argc, char *argv[]) {
    peArena temp_arena;
    {
        size_t temp_arena_size = PE_MEGABYTES(4);
        pe_arena_init(&temp_arena, pe_heap_alloc(temp_arena_size), temp_arena_size);
    }

    pe_platform_init();
    pe_graphics_init(&temp_arena, 960, 540, "Procyon");
    pe_input_init();
    pe_time_init();

#if !defined(PSP)
    #define PE_MODEL_EXTENSION ".p3d"
#else
    #define PE_MODEL_EXTENSION ".pp3d"
#endif
    peModel model = pe_model_load(&temp_arena, "./res/fox" PE_MODEL_EXTENSION);

    // peCamera camera = {
    //     .target = {0.0f, 0.7f, 0.0f},
    //     .up = {0.0f, 1.0f, 0.0f},
    //     .fovy = 55.0f,
    // };
    // HMM_Vec3 camera_offset = HMM_V3(0.0f, 1.4f, 2.0f);
    peCamera camera = {
        .target = {0.0f, 0.0f, -4.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    HMM_Vec3 camera_offset = HMM_V3(0.0f, 5.0f, 10.0f);
    camera.position = HMM_AddV3(camera.target, camera_offset);

    float time_since_last_print = 0.0f;

    while(!pe_platform_should_quit()) {
        pe_profile_reset();
        pe_profile_region_begin(peProfileRegion_GameLoop);

        uint64_t frame_start_time = pe_time_now();

        pe_platform_poll_events();
        pe_input_update();

        pe_camera_update(camera);

        pe_graphics_frame_begin();
        pe_clear_background((peColor){ 20, 20, 20, 255 });

        {
            HMM_Vec2 draw_x_range = HMM_V2(-5.0f, 5.0f);
            HMM_Vec2 draw_z_range = HMM_V2(-12.0f, 2.0f);
            int models_to_draw_x = 4;
            int models_to_draw_z = 4;

            float diff_x = (draw_x_range.Elements[1] - draw_x_range.Elements[0]) / (float)(models_to_draw_x - 1);
            float diff_z = (draw_z_range.Elements[1] - draw_z_range.Elements[0]) / (float)(models_to_draw_z - 1);

            for (int z = 0; z < models_to_draw_z; z += 1) {
                for (int x = 0; x < models_to_draw_x; x += 1) {
                    pe_profile_region_begin(peProfileRegion_ModelDraw);
                    float position_x = draw_x_range.Elements[0] + ((float)x * diff_x);
                    float position_z = draw_z_range.Elements[0] + ((float)z * diff_z);
                    HMM_Vec3 position = HMM_V3(position_x, 0.0f, position_z);
                    pe_model_draw(&model, &temp_arena, position, HMM_V3(0.0f, 0.0f, 0.0f));
                    pe_profile_region_end(peProfileRegion_ModelDraw);
                }
            }
        }

        // pe_model_draw(&model, &temp_arena, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 0.0f, 0.0f));

		pe_profile_region_begin(peProfileRegion_FrameEnd);
        pe_graphics_frame_end(false);
		pe_profile_region_end(peProfileRegion_FrameEnd);

        pe_arena_clear(&temp_arena);

        pe_profile_region_end(peProfileRegion_GameLoop);

        uint64_t frame_time = pe_time_since(frame_start_time);
        time_since_last_print += (float)pe_time_sec(frame_time);
        if (time_since_last_print > 2.0f) {
            fprintf(stdout, "PROFILE DATA:\n");
            for (int r = 0; r < peProfileRegion_Count; r += 1) {
                peProfileRecord pr = pe_profile_record_get(r);
                double pr_ms = pe_time_ms(pr.time_spent);
                //fprintf(stdout, "%2d: total=%.3f, count=%u, avg=%.4f\n", r, pr_ms, pr.count, pr_ms/(double)pr.count);
            }
            time_since_last_print = 0.0f;
        }
    }

    pe_graphics_shutdown();
    pe_platform_shutdown();

    return 0;
}