#include "pe_core.h"
#include "pe_platform.h"
#include "pe_graphics.h"
#include "pe_temp_arena.h"

#if defined(_WIN32)
#include "pe_window_glfw.h"
#endif

int main(int argc, char* argv[]) {
	pe_temp_allocator_init(PE_MEGABYTES(4));
	pe_platform_init();
    pe_graphics_init(960, 540, "Procyon");

	// do model loading here

	while (!pe_platform_should_quit()) {
		pe_platform_poll_events();

		pe_graphics_frame_begin();
		pe_clear_background(PE_COLOR_BLACK);

		// do model rendering here

		pe_graphics_frame_end(true);
		pe_free_all(pe_temp_arena());
	}

#if defined(_WIN32)
	pe_glfw_shutdown();
#endif

    return 0;
}