#include "pe_graphics_linux.h"

#include "pe_core.h"
#include "pe_window_glfw.h"

#include "glad/glad.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <stdio.h>

static void glfw_framebuffer_size_proc(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    pe_glfw.window_width = width;
    pe_glfw.window_height = height;
}

static void gl_debug_message_proc(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const void *userParam
) {
    fprintf(stderr, "GL error, source=%d type=%d id=%u severity=%d %s", source, type, id, severity, message);
}

void pe_graphics_init_linux(void) {
    int window_width, window_height;

    // init GLFW
    {
        PE_ASSERT(pe_glfw.initialized);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        glfwGetWindowSize(pe_glfw.window, &window_width, &window_height);
    }

    // TODO: Confirm that glDebugMessageCallbackARB actually works
#if !defined(NDEBUG) && defined(GL_ARB_debug_output)
    if (glDebugMessageCallbackARB) {
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_message_proc, NULL);
    }
#endif

    glViewport(0, 0, window_width, window_height);
}
