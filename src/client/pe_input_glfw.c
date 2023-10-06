#include "pe_input_glfw.h"

#include "pe_window_glfw.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <stdbool.h>

#define PE_GLFW_KEY_COUNT (GLFW_KEY_LAST+1)

struct peInputGLFWState {
    bool key_pressed[PE_GLFW_KEY_COUNT];
    int key_state_changes[PE_GLFW_KEY_COUNT];
} pe_input_glfw_state = {0};

static void pe_glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {

}

void pe_input_init_glfw(void) {
    PE_ASSERT(pe_glfw.initialized);
    glfwSetKeyCallback(pe_glfw.window, pe_glfw_key_callback);
}