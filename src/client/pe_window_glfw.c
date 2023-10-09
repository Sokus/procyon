#include "pe_window_glfw.h"

#include "pe_input_glfw.h"
#include "pe_core.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include "GLFW/glfw3native.h"

    #include "pe_graphics_win32.h"
#elif defined(__linux__)
	#include "glad/glad.h"

	#include "pe_graphics_linux.h"
#endif

#include <stdbool.h>

peGLFW pe_glfw = {0};

static void pe_framebuffer_size_callback_glfw(GLFWwindow *window, int width, int height) {
#if defined(_WIN32)
    pe_framebuffer_size_callback_win32(width, height);
#elif defined(__linux__)
    pe_framebuffer_size_callback_linux(width, height);
#endif
}

static void pe_window_key_callback_glfw(GLFWwindow *window, int key, int scancode, int action, int mods) {
    pe_input_key_callback_glfw(key, action);
}

static void pe_cursor_position_callback_glfw(GLFWwindow *window, double pos_x, double pos_y) {
    pe_input_cursor_position_callback_glfw(pos_x, pos_y);
}

void pe_glfw_init(int window_width, int window_height, const char *window_name) {
    if (pe_glfw.initialized == false) {
        glfwInit();

        // set window hints
        {
            glfwWindowHint(GLFW_SCALE_TO_MONITOR, 0);
#if defined(_WIN32)
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#elif defined(__linux__)
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
#if defined(__linux__) && !defined(NDEBUG)
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
        }

        pe_glfw.window = glfwCreateWindow(window_width, window_height, window_name, NULL, NULL);

        // init graphics
        {
#if defined(_WIN32)
            HWND hwnd = glfwGetWin32Window(pe_glfw.window);
            pe_graphics_init_win32(hwnd, window_width, window_height);
#elif defined(__linux__)
            glfwMakeContextCurrent(pe_glfw.window);
            gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
            pe_graphics_init_linux(window_width, window_height);
#endif
        }

        glfwSetFramebufferSizeCallback(pe_glfw.window, pe_framebuffer_size_callback_glfw);
        glfwSetKeyCallback(pe_glfw.window, pe_window_key_callback_glfw);
        glfwSetCursorPosCallback(pe_glfw.window, pe_cursor_position_callback_glfw);

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