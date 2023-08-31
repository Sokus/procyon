#include "pe_window_glfw.h"

#include "pe_core.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <stdbool.h>

peGLFW pe_glfw = {0};

void pe_glfw_init(int window_width, int window_height, const char *window_name) {
    if (pe_glfw.initialized == false) {
        glfwInit();

        glfwWindowHint(GLFW_SCALE_TO_MONITOR, 0);
#if defined(_WIN32)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#elif defined(__linux__)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

        pe_glfw.window = glfwCreateWindow(window_width, window_height, window_name, NULL, NULL);
        pe_glfw.window_width = window_width;
        pe_glfw.window_height = window_height;

#if defined(__linux__)
        glfwMakeContextCurrent(pe_glfw.window);
#endif

        pe_glfw.initialized = true;
    }
}

void pe_glfw_shutdown(void) {
    PE_ASSERT(pe_glfw.initialized);
    glfwDestroyWindow(pe_glfw.window);
    glfwTerminate();
    pe_glfw.initialized = false;
}

bool pe_window_should_quit_glfw(void) {
    return glfwWindowShouldClose(pe_glfw.window);
}

void pe_window_poll_events_glfw(void) {
    glfwPollEvents();
}