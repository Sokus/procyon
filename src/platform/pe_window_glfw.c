#include "pe_window_glfw.h"

#include "pe_input_glfw.h"
#include "core/p_assert.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include "GLFW/glfw3native.h"

    #include "graphics/pe_graphics_win32.h"
#elif defined(__linux__)
	#include "glad/glad.h"

	#include "graphics/pe_graphics_linux.h"
#endif

#include <stdbool.h>

struct peWindowGLFW {
    GLFWwindow *window;
    peWindowFramebufferSizeCallback *framebuffer_size_callback;
    peWindowKeyCallback *key_callback;
    peWindowCursorPositionCallback *cursor_position_callback;
};
struct peWindowGLFW window_state_glfw = {0};

static void pe_framebuffer_size_callback_glfw(GLFWwindow *window, int width, int height) {
    P_ASSERT(window_state_glfw.framebuffer_size_callback != NULL);
    window_state_glfw.framebuffer_size_callback(width, height);
}

static void pe_window_key_callback_glfw(GLFWwindow *window, int key, int scancode, int action, int mods) {
    P_ASSERT(window_state_glfw.key_callback != NULL);
    window_state_glfw.key_callback(key, action);
}

static void pe_cursor_position_callback_glfw(GLFWwindow *window, double pos_x, double pos_y) {
    P_ASSERT(window_state_glfw.cursor_position_callback != NULL);
    window_state_glfw.cursor_position_callback(pos_x, pos_y);
}

void pe_window_init_glfw(int window_width, int window_height, const char *window_name) {
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

    window_state_glfw.window = glfwCreateWindow(window_width, window_height, window_name, NULL, NULL);

    // init graphics
    {
#if defined(__linux__)
        glfwMakeContextCurrent(window_state_glfw.window);
#endif
    }
}

void pe_window_shutdown_glfw(void) {
    glfwDestroyWindow(window_state_glfw.window);
    glfwTerminate();
}

void pe_window_set_framebuffer_size_callback_glfw(peWindowFramebufferSizeCallback *cb) {
    window_state_glfw.framebuffer_size_callback = cb;
    glfwSetFramebufferSizeCallback(window_state_glfw.window, pe_framebuffer_size_callback_glfw);
}

void pe_window_set_key_callback_glfw(peWindowKeyCallback *cb) {
    window_state_glfw.key_callback = cb;
    glfwSetKeyCallback(window_state_glfw.window, pe_window_key_callback_glfw);
}

void pe_window_set_cursor_position_callback_glfw(peWindowCursorPositionCallback *cb) {
    window_state_glfw.cursor_position_callback = cb;
    glfwSetCursorPosCallback(window_state_glfw.window, pe_cursor_position_callback_glfw);
}

bool pe_window_should_quit_glfw(void) {
    return glfwWindowShouldClose(window_state_glfw.window);
}

void pe_window_poll_events_glfw(void) {
    glfwPollEvents();
}

#if defined(_WIN32)
HWND pe_window_get_win32_window_glfw(void) {
    return glfwGetWin32Window(window_state_glfw.window);
}
#endif

#if defined(__linux__)
void *pe_window_get_proc_address_glfw(const char *name) {
    return glfwGetProcAddress(name);
}

void pe_window_swap_buffers_glfw(bool vsync) {
	int interval = (vsync ? 1 : 0);
	glfwSwapInterval(interval);
	glfwSwapBuffers(window_state_glfw.window);
}
#endif