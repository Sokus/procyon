#ifndef PE_WINDOW_GLFW_H_HEADER_GUARD
#define PE_WINDOW_GLFW_H_HEADER_GUARD

#include <stdbool.h>


#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__linux__)
    #include "glad/glad.h"
#endif

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

typedef void peWindowKeyCallback(int key, int scancode);
typedef void peWindowCursorPositionCallback(double pos_x, double pos_y);

typedef struct peGLFW {
    bool initialized;
    GLFWwindow *window;

    peWindowKeyCallback *key_callback;
    peWindowCursorPositionCallback *cursor_position_callback;
} peGLFW;
extern peGLFW pe_glfw;


struct peArena;
void pe_glfw_init(struct peArena *temp_arena, int window_width, int window_height, const char *window_name);
void pe_glfw_shutdown(void);

void pe_window_set_key_callback(peWindowKeyCallback *cb);
void pe_window_set_cursor_position_callback(peWindowCursorPositionCallback *cb);

bool pe_window_should_quit_glfw(void);
void pe_window_poll_events_glfw(void);

#if defined(_WIN32)
HWND pe_glfw_get_win32_window(void);
#endif

#endif // PE_WINDOW_GLFW_H_HEADER_GUARD