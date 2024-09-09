#include "pe_window.h"

#include "core/p_assert.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#if defined(_WIN32) || defined(__linux__)
    #include "pe_window_glfw.h"
#endif

#include <stdbool.h>

struct peWindowState {
    bool initialized;
};
struct peWindowState window_state = {0};

void pe_window_init(int window_width, int window_height, const char *window_name) {
    P_ASSERT(!window_state.initialized);
#if defined(_WIN32) || defined(__linux__)
    pe_window_init_glfw(window_width, window_height, window_name);
#endif
    window_state.initialized = true;
}

void pe_window_shutdown(void) {
    P_ASSERT(window_state.initialized);
#if defined(_WIN32) || defined(__linux__)
    pe_window_shutdown_glfw();
#endif
    window_state.initialized = false;
}

void pe_window_set_framebuffer_size_callback(peWindowFramebufferSizeCallback *cb) {
    P_ASSERT(window_state.initialized);
#if defined(_WIN32) || defined(__linux__)
    pe_window_set_framebuffer_size_callback_glfw(cb);
#endif
}

void pe_window_set_key_callback(peWindowKeyCallback *cb) {
    P_ASSERT(window_state.initialized);
#if defined(_WIN32) || defined(__linux__)
    pe_window_set_key_callback_glfw(cb);
#endif
}

void pe_window_set_cursor_position_callback(peWindowCursorPositionCallback *cb) {
    P_ASSERT(window_state.initialized);
#if defined(_WIN32) || defined(__linux__)
    pe_window_set_cursor_position_callback_glfw(cb);
#endif
}

bool pe_window_should_quit(void) {
    bool result = false;
#if defined(_WIN32) || defined(__linux__)
    result = pe_window_should_quit_glfw();
#endif
    return result;
}

void pe_window_poll_events(void) {
#if defined(_WIN32) || defined(__linux__)
    pe_window_poll_events_glfw();
#endif
}

#if defined(_WIN32)
HWND pe_window_get_win32_window(void) {
    return pe_window_get_win32_window_glfw();
}
#endif

#if defined(__linux__)
void *pe_window_get_proc_address(const char *name) {
    return pe_window_get_proc_address_glfw(name);
}

void pe_window_swap_buffers(bool vsync) {
    pe_window_swap_buffers_glfw(vsync);
}
#endif