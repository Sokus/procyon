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

typedef struct peGLFW {
    bool initialized;
    GLFWwindow *window;

    bool key_pressed[GLFW_KEY_LAST];
    int key_state_changes[GLFW_KEY_LAST];
} peGLFW;
extern peGLFW pe_glfw;

void pe_glfw_init(int window_width, int window_height, const char *window_name);
void pe_glfw_shutdown(void);

bool pe_window_should_quit_glfw(void);
void pe_window_poll_events_glfw(void);

#if defined(_WIN32)
HWND pe_glfw_get_win32_window(void);
#endif

#endif // PE_WINDOW_GLFW_H_HEADER_GUARD