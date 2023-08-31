#include "pe_graphics_linux.h"

#include "pe_core.h"
#include "pe_window_glfw.h"

#include "glad/glad.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"


static void glfw_framebuffer_size_proc(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    pe_glfw.window_width = width;
    pe_glfw.window_height = height;
}

void pe_graphics_init_linux(void) {
    int window_width, window_height;

    // init GLFW
    {
        PE_ASSERT(pe_glfw.initialized);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        glfwGetWindowSize(pe_glfw.window, &window_width, &window_height);
    }

    glViewport(0, 0, window_width, window_height);
}
