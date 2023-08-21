#ifndef PE_WIN32_GLFW_H
#define PE_WIN32_GLFW_H

#include <stdbool.h>

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


#endif // PE_WIN32_GLFW_H