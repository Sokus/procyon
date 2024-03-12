#ifndef PE_WINDOW_GLFW_H_HEADER_GUARD
#define PE_WINDOW_GLFW_H_HEADER_GUARD

#include "pe_window.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__linux__)
    #include "glad/glad.h"
#endif

#include <stdbool.h>

struct peArena;
void pe_window_init_glfw(int window_width, int window_height, const char *window_name);
void pe_window_shutdown_glfw(void);

void pe_window_set_framebuffer_size_callback_glfw(peWindowFramebufferSizeCallback *cb);
void pe_window_set_key_callback_glfw(peWindowKeyCallback *cb);
void pe_window_set_cursor_position_callback_glfw(peWindowCursorPositionCallback *cb);

bool pe_window_should_quit_glfw(void);
void pe_window_poll_events_glfw(void);

#if defined(_WIN32)
HWND pe_window_get_win32_window_glfw(void);
#endif

#if defined(__linux__)
void *pe_window_get_proc_address_glfw(const char *name);
void pe_window_swap_buffers_glfw(bool vsync);
#endif

#endif // PE_WINDOW_GLFW_H_HEADER_GUARD