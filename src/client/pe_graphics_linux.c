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

static const char *gl_debug_message_source_to_string(GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API_ARB:               return "GL_DEBUG_SOURCE_API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:     return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:   return "GL_DEBUG_SOURCE_SHADER_COMPILE";
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:       return "GL_DEBUG_SOURCE_THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION_ARB:       return "GL_DEBUG_SOURCE_APPLICATION";
        case GL_DEBUG_SOURCE_OTHER_ARB:             return "GL_DEBUG_SOURCE_OTHER";
        default:                                    break;
    }
    return "GL_DEBUG_SOURCE_???";
}

static const char *gl_debug_message_type_to_string(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR_ARB:               return "GL_DEBUG_TYPE_ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY_ARB:         return "GL_DEBUG_TYPE_PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE_ARB:         return "GL_DEBUG_TYPE_PERFORMANCE";
        case GL_DEBUG_TYPE_OTHER_ARB:               return "GL_DEBUG_TYPE_OTHER";
        default:                                    break;
    }
    return "GL_DEBUG_TYPE_???";
}

static const char *gl_debug_message_severity_to_string(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH_ARB:    return "GL_DEBUG_SEVERITY_HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:  return "GL_DEBUG_SEVERITY_MEDIUM";
        case GL_DEBUG_SEVERITY_LOW_ARB:     return "GL_DEBUG_SEVERITY_LOW";
        default:                            break;
    }
    return "GL_DEBUG_SEVERITY_???";
}

static void gl_debug_message_proc(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar *message, const void *userParam
) {
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    printf(
        "GL_DEBUG [%d], %s [0x%x], %s [0x%x], %s [0x%x]:\n%s\n",
        id,
        gl_debug_message_source_to_string(source), source,
        gl_debug_message_type_to_string(type), type,
        gl_debug_message_severity_to_string(severity), severity,
        message
    );
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
