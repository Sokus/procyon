#include "pe_core.h"
#include "client/pe_platform.h"
#include "client/pe_graphics.h"
#include "pe_temp_arena.h"
#include "client/pe_input.h"
#include "client/pe_model.h"
#include "pe_time.h"

#include <stdio.h>

#include "HandmadeMath.h"

int main(int argc, char *argv[]) {
	pe_temp_arena_init(PE_MEGABYTES(4));

    pe_platform_init();
    pe_graphics_init(960, 540, "Procyon");
    pe_input_init();
    pe_time_init();

#if !defined(PSP)
    #define PE_MODEL_EXTENSION ".p3d"
#else
    #define PE_MODEL_EXTENSION ".pp3d"
#endif
    peModel model = pe_model_load("./res/fox" PE_MODEL_EXTENSION);

    peCamera camera = {
        .target = {0.0f, 0.7f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    HMM_Vec3 camera_offset = HMM_V3(0.0f, 1.4f, 2.0f);
    camera.position = HMM_AddV3(camera.target, camera_offset);

    while(!pe_platform_should_quit()) {
        pe_platform_poll_events();
        pe_input_update();

        pe_camera_update(camera);

        pe_graphics_frame_begin();
        pe_clear_background((peColor){ 20, 20, 20, 255 });

        pe_model_draw(&model, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 0.0f, 0.0f));

        pe_graphics_frame_end(true);

        pe_arena_clear(pe_temp_arena());
    }

    pe_graphics_shutdown();
    pe_platform_shutdown();

    return 0;
}