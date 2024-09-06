#include "core/pe_core.h"
#include "platform/pe_platform.h"
#include "graphics/pe_graphics.h"
#include "platform/pe_input.h"
#include "graphics/pe_model.h"
#include "platform/p_time.h"
#include "utility/pe_trace.h"
#include "platform/pe_window.h"

#include <stdio.h>

#include "HandmadeMath.h"

int main(int argc, char *argv[]) {
    peArena temp_arena;
    {
        size_t temp_arena_size = PE_MEGABYTES(4);
        pe_arena_init(&temp_arena, pe_heap_alloc(temp_arena_size), temp_arena_size);
    }

    pe_platform_init();
    pe_window_init(960, 540, "Procyon");
    pe_graphics_init(&temp_arena, 960, 540);
    pe_input_init();
    ptime_init();
    pe_trace_init();

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
    pVec3 camera_offset = p_vec3(0.0f, 5.0f, 10.0f);
    camera.position = p_vec3_add(camera.target, camera_offset);

    float average_frame_duration = 0.0f;

    while(true) {
        bool should_quit = (
            pe_platform_should_quit() ||
            pe_window_should_quit()
        );
        if (should_quit) {
            break;
        }

        uint64_t frame_start_time = ptime_now();
        peTraceMark tm_game_loop = PE_TRACE_MARK_BEGIN("game loop");

        pe_window_poll_events();
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
                    float position_x = draw_x_range.Elements[0] + ((float)x * diff_x);
                    float position_z = draw_z_range.Elements[0] + ((float)z * diff_z);
                    pVec3 position = p_vec3(position_x, 0.0f, position_z);
                    pe_model_draw(&model, &temp_arena, position, p_vec3(0.0f, 0.0f, 0.0f));
                }
            }
        }

        // pe_model_draw(&model, &temp_arena, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 0.0f, 0.0f));

        pe_graphics_frame_end(false);

        pe_arena_clear(&temp_arena);

        PE_TRACE_MARK_END(tm_game_loop);
        uint64_t frame_duration = ptime_since(frame_start_time);
        float frame_duration_ms = (float)ptime_ms(frame_duration);
        average_frame_duration += 0.05f * (frame_duration_ms - average_frame_duration);
        // printf("average frame duration: %6.3f\n", average_frame_duration);
    }


    pe_trace_shutdown();
    pe_graphics_shutdown();
    pe_window_shutdown();
    pe_platform_shutdown();

    return 0;
}