#include "pe_core.h"
#include "pe_platform.h"
#include "pe_graphics.h"
#include "pe_temp_arena.h"
#include "pe_model.h"
#include "pe_time.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if defined(_WIN32) || defined(__linux__)
	#define PE_MODEL_EXTENSION "p3d"
#elif defined(PSP)
	#define PE_MODEL_EXTENSION "pp3d"
#endif

int main(int argc, char* argv[]) {
	pe_temp_arena_init(PE_MEGABYTES(4));
	pe_platform_init();
    pe_graphics_init(960, 540, "Procyon");
	pe_time_init();

	peCamera camera = {
		.position = { 0.0f, 2.1f, 2.0f },
		.target = { 0.0f, 0.7f, 0.0f },
		.up = { 0.0f, 1.0f, 0.0f },
		.fovy = 55.0f,
	};

    peModel model = pe_model_load("./res/fox." PE_MODEL_EXTENSION);

	while (!pe_platform_should_quit()) {
		pe_platform_poll_events();

		pe_graphics_frame_begin();
		pe_clear_background((peColor){ 20, 20, 20, 255 });

		pe_camera_update(camera);

		pe_model_draw(&model, (HMM_Vec3){0.0f}, HMM_V3(0.0f, HMM_PI32, 0.0f));

		pe_graphics_frame_end(true);
		pe_arena_clear(pe_temp_arena());
	}

	pe_graphics_shutdown();
	pe_platform_shutdown();

    return 0;
}