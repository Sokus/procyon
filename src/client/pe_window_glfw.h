#ifndef PE_WINDOW_GLFW_H_HEADER_GUARD
#define PE_WINDOW_GLFW_H_HEADER_GUARD

#include <stdbool.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

typedef struct peGLFW {
    bool initialized;
    GLFWwindow *window;
    int window_width;
    int window_height;
} peGLFW;

extern peGLFW pe_glfw;

void pe_glfw_init(int window_width, int window_height, const char *window_name);
void pe_glfw_shutdown(void);

bool pe_window_should_quit_glfw(void);
void pe_window_poll_events_glfw(void);

#endif // PE_WINDOW_GLFW_H_HEADER_GUARD