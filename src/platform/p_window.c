#include "p_window.h"

#include "core/p_arena.h"
#include "core/p_assert.h"
#include "core/p_time.h"

#include "platform/p_input.h"
#include "graphics/pe_graphics.h"

#include <stdbool.h>

struct pWindowState {
    bool initialized;
    int target_fps;
    struct {
        uint64_t start;
        uint64_t last_tick;
        uint64_t update_ticks;
        uint64_t draw_ticks;
    } time;
} p_window_state = {0};

void pe_window_set_target_fps(int fps) {
    P_ASSERT(fps > 0);
    p_window_state.target_fps = fps;
}

float pe_window_delta_time(void) {
    int target_fps = (p_window_state.target_fps ? p_window_state.target_fps : 60);
    return 1.0f / (float)target_fps;
}

void pe_window_init(peArena *temp_arena, int window_width, int window_height, const char *window_name) {
    P_ASSERT(!p_window_state.initialized);
    uint64_t time_now = p_time_now();
    p_window_state.time.start = time_now;
    p_window_state.time.last_tick = time_now;

    pe_window_platform_init(window_width, window_height, window_name);
    pe_graphics_init(temp_arena, window_width, window_height);
    pe_input_init();

    p_window_state.initialized = true;
}

void pe_window_shutdown(void) {
    P_ASSERT(p_window_state.initialized);
    pe_graphics_shutdown();
    pe_window_platform_shutdown();
    p_window_state.initialized = false;
}

void pe_window_frame_begin(void) {
    uint64_t time_now = p_time_now();
    p_window_state.time.update_ticks = p_time_diff(time_now, p_window_state.time.last_tick);
    p_window_state.time.last_tick = time_now;

    pe_graphics_frame_begin();
}

static void pe_window_frame_sync(bool vsync) {
    uint64_t loop_time_ticks = p_window_state.time.update_ticks + p_window_state.time.draw_ticks;
    double loop_time_sec = p_time_sec(loop_time_ticks);
    float dt = pe_window_delta_time();
    if (vsync && loop_time_sec < dt) {
        unsigned long sleep_time_ms = (unsigned long)((dt - loop_time_sec) * 1000.0f);
        p_time_sleep(sleep_time_ms);
        p_window_state.time.last_tick = p_time_now();
    }
}

void pe_window_frame_end(bool vsync) {
	pe_graphics_dynamic_draw_draw_batches();
	pe_graphics_frame_end();
	pe_window_swap_buffers(vsync);
	pe_graphics_dynamic_draw_clear();

    uint64_t time_now = p_time_now();
    p_window_state.time.draw_ticks = p_time_diff(time_now, p_window_state.time.last_tick);
    p_window_state.time.last_tick = time_now;
    pe_window_frame_sync(vsync);

    pe_input_update();
    pe_window_poll_events();
}
