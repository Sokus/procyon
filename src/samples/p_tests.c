#include "core/p_heap.h"
#include "core/p_time.h"
#include "core/p_scratch.h"
#include "graphics/p_graphics.h"
#include "graphics/p_model.h"
#include "platform/p_input.h"
#include "platform/p_window.h"
#include "utility/p_trace.h"

#include <stdio.h>

#include "HandmadeMath.h"

int main(int argc, char *argv[]) {
    p_window_init(960, 540, "Procyon");
    p_trace_init();

#if !defined(PSP)
    #define PE_MODEL_EXTENSION ".p3d"
#else
    #define PE_MODEL_EXTENSION ".pp3d"
#endif
    pModel model = p_model_load("./res/fox" PE_MODEL_EXTENSION);

    // pCamera camera = {
    //     .target = {0.0f, 0.7f, 0.0f},
    //     .up = {0.0f, 1.0f, 0.0f},
    //     .fovy = 55.0f,
    // };
    // HMM_Vec3 camera_offset = HMM_V3(0.0f, 1.4f, 2.0f);
    pCamera camera = {
        .target = {0.0f, 0.0f, -4.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    pVec3 camera_offset = p_vec3(0.0f, 5.0f, 10.0f);
    camera.position = p_vec3_add(camera.target, camera_offset);

    float average_frame_duration = 0.0f;

    while(p_window_should_quit()) {
        uint64_t frame_start_time = p_time_now();
        pTraceMark tm_game_loop = P_TRACE_MARK_BEGIN("game loop");


        // p_camera_update(camera);

        p_window_frame_begin();
        p_clear_background((pColor){ 20, 20, 20, 255 });

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
                    p_model_draw(&model, position, p_vec3(0.0f, 0.0f, 0.0f));
                }
            }
        }

        // p_model_draw(&model, &temp_arena, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 0.0f, 0.0f));

        p_window_frame_end(false);

        p_scratch_clear();

        P_TRACE_MARK_END(tm_game_loop);
        uint64_t frame_duration = p_time_since(frame_start_time);
        float frame_duration_ms = (float)p_time_ms(frame_duration);
        average_frame_duration += 0.05f * (frame_duration_ms - average_frame_duration);
        // printf("average frame duration: %6.3f\n", average_frame_duration);
    }

    p_trace_shutdown();
    p_graphics_shutdown();
    p_window_shutdown();

    return 0;
}
