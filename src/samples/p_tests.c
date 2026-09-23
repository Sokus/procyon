#include "core/p_heap.h"
#include "core/p_time.h"
#include "core/p_scratch.h"
#include "graphics/p_graphics.h"
#include "graphics/p_model.h"
#include "platform/p_input.h"
#include "platform/p_window.h"
#include "utility/p_trace.h"

#if defined(__PSP__)
#include <pspdebug.h>
#endif

#include <stdio.h>

int main(int argc, char *argv[]) {
    p_window_init(960, 540, "Procyon");
    p_trace_init();

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

    while(!p_window_should_quit()) {
        uint64_t frame_start_time = p_time_now();
        pTraceMark tm_game_loop = P_TRACE_MARK_BEGIN("game loop");
        // p_camera_update(camera);

        p_window_frame_begin();
#if defined(__PSP__)
        pspDebugScreenClear();
        pspDebugScreenPrintf("bla bla\n");
#endif
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
