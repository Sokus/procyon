#ifndef PE_WINDOW_H_HEADER_GUARD
#define PE_WINDOW_H_HEADER_GUARD

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <stdbool.h>

typedef void peWindowFramebufferSizeCallback(int width, int height);
typedef void peWindowKeyCallback(int key, int scancode);
typedef void peWindowCursorPositionCallback(double pos_x, double pos_y);

void pe_window_init(int window_width, int window_height, const char *window_name);
void pe_window_shutdown(void);

void pe_window_set_framebuffer_size_callback(peWindowFramebufferSizeCallback *cb);
void pe_window_set_key_callback(peWindowKeyCallback *cb);
void pe_window_set_cursor_position_callback(peWindowCursorPositionCallback *cb);

bool pe_window_should_quit(void);
void pe_window_poll_events(void);

#if defined(_WIN32)
HWND pe_window_get_win32_window(void);
#endif

#if defined(__linux__)
void *pe_window_get_proc_address(const char *name);
void pe_window_swap_buffers(bool vsync);
#endif

#endif // PE_WINDOW_H_HEADER_GUARD