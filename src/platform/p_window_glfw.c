#include "platform/p_window.h"

#include "core/p_assert.h"
#include "graphics/p_graphics.h"
#include "platform/p_input.h"
#include "utility/p_trace.h"
#if defined(_WIN32)
    #include "graphics/p_graphics_win32.h"
#endif

#include <stdbool.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include "GLFW/glfw3native.h"

    #define COBJMACROS
    #include <dxgi1_2.h>
    #include <d3d11.h>
#elif defined(__linux__)
	#include "glad/glad.h"
#endif

struct pWindowStateGLFW {
    GLFWwindow *window;
    bool should_quit;
} p_window_state_glfw = {0};

static void p_window_close_callback(GLFWwindow *window);
static void p_window_framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void p_window_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void p_window_cursor_position_callback(GLFWwindow *window, double pos_x, double pos_y);

void p_window_platform_init(int window_width, int window_height, const char *window_name) {
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

    p_window_state_glfw.window = glfwCreateWindow(window_width, window_height, window_name, NULL, NULL);

    // init graphics
#if defined(_WIN32)
    p_hwnd = glfwGetWin32Window(p_window_state_glfw.window);
#endif
#if defined(__linux__)
    glfwMakeContextCurrent(p_window_state_glfw.window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#endif

    glfwSetWindowCloseCallback(p_window_state_glfw.window, p_window_close_callback);
    glfwSetFramebufferSizeCallback(p_window_state_glfw.window, p_window_framebuffer_size_callback);
    glfwSetKeyCallback(p_window_state_glfw.window, p_window_key_callback);
    glfwSetCursorPosCallback(p_window_state_glfw.window, p_window_cursor_position_callback);
}

void p_window_platform_shutdown(void) {
    glfwDestroyWindow(p_window_state_glfw.window);
    glfwTerminate();
}

bool p_window_should_quit(void) {
    return p_window_state_glfw.should_quit;
}

void p_window_poll_events(void) {
    glfwPollEvents();
}

void p_window_swap_buffers(bool vsync) {
    P_TRACE_FUNCTION_BEGIN();
#if defined(_WIN32)
    UINT sync_interval = (vsync ? 1 : 0);
    IDXGISwapChain1_Present(p_d3d.swapchain, sync_interval, 0);
    ID3D11DeviceContext_OMSetRenderTargets(p_d3d.context, 1, &p_d3d.render_target_view, p_d3d.depth_stencil_view);
#endif
#if defined(__linux__)
    int interval = (vsync ? 1 : 0);
	glfwSwapInterval(interval);
	glfwSwapBuffers(p_window_state_glfw.window);
#endif
    P_TRACE_FUNCTION_END();
}

////////////////////////////////////////////////////////////////////////////////

static void p_window_close_callback(GLFWwindow *window) {
    p_window_state_glfw.should_quit = true;
}

static void p_window_framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    p_graphics_set_framebuffer_size(width, height);
}

static void p_window_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    p_input_key_callback(key, action);
}

static void p_window_cursor_position_callback(GLFWwindow *window, double pos_x, double pos_y) {
    p_input_cursor_position_callback(pos_x, pos_y);
}
