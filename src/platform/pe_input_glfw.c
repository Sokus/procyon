#include "pe_input_glfw.h"
#include "pe_input.h"

#include "core/p_defines.h"
#include "core/p_assert.h"

#include "platform/pe_window.h"


#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <stdbool.h>
#include <string.h>

#define PE_GLFW_KEY_COUNT (GLFW_KEY_LAST+1)

typedef struct peKeyboardStateGLFW {
    bool key_down[PE_GLFW_KEY_COUNT];
    int key_state_changes[PE_GLFW_KEY_COUNT];
} peKeyboardStateGLFW;

typedef struct peMouseStateGLFW {
    double pos_x;
    double pos_y;
} peMouseStateGLFW;

typedef struct peGamepadStateGLFW {
    bool button_current[peGamepadButton_Count];
    bool button_last[peGamepadButton_Count];
    float axis[peGamepadAxis_Count];
} peGamepadStateGLFW;

struct peInputStateGLFW {
    struct {
        peKeyboardStateGLFW current;
        peKeyboardStateGLFW next;
    } keyboard_state;

    peMouseStateGLFW mouse_state;
    peGamepadStateGLFW gamepad_state;
} pe_input_state_glfw = {0};

static void pe_input_key_callback_glfw(int key, int action) {
    if (action != GLFW_PRESS && action != GLFW_RELEASE) {
        return;
    }
    bool new_key_state = (action == GLFW_PRESS);
    P_ASSERT(new_key_state != pe_input_state_glfw.keyboard_state.next.key_down[key]);
    pe_input_state_glfw.keyboard_state.next.key_down[key] = new_key_state;
    pe_input_state_glfw.keyboard_state.next.key_state_changes[key] += 1;
}

static void pe_input_cursor_position_callback_glfw(double pos_x, double pos_y) {
    pe_input_state_glfw.mouse_state.pos_x = pos_x;
    pe_input_state_glfw.mouse_state.pos_y = pos_y;
}

void pe_input_init_glfw(void) {
    pe_window_set_key_callback(&pe_input_key_callback_glfw);
    pe_window_set_cursor_position_callback(&pe_input_cursor_position_callback_glfw);
}

void pe_input_update_glfw(void) {
    // update keyboard state
    {
        peKeyboardStateGLFW *keyboard_state_current = &pe_input_state_glfw.keyboard_state.current;
        peKeyboardStateGLFW *keyboard_state_next = &pe_input_state_glfw.keyboard_state.next;
        memcpy(keyboard_state_current, keyboard_state_next, sizeof(peKeyboardStateGLFW));
        memset(keyboard_state_next->key_state_changes, 0, sizeof(keyboard_state_next->key_state_changes));
    }

    // update gamepad state
    {
        peGamepadStateGLFW *gamepad_state = &pe_input_state_glfw.gamepad_state;
        memcpy(gamepad_state->button_last, gamepad_state->button_current, sizeof(gamepad_state->button_current));
        memset(gamepad_state->button_current, 0, sizeof(gamepad_state->button_current));
        memset(gamepad_state->axis, 0, sizeof(gamepad_state->axis));

        for (int joystick_id = GLFW_JOYSTICK_1; joystick_id < GLFW_JOYSTICK_LAST; joystick_id += 1) {
            GLFWgamepadstate glfw_gamepad_state;
            if (GLFW_FALSE == glfwGetGamepadState(joystick_id, &glfw_gamepad_state)) {
                continue;
            }

	        gamepad_state->button_current[peGamepadButton_ActionDown] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_A];
	        gamepad_state->button_current[peGamepadButton_ActionRight] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_B];
	        gamepad_state->button_current[peGamepadButton_ActionLeft] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_X];
	        gamepad_state->button_current[peGamepadButton_ActionUp] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_Y];
	        gamepad_state->button_current[peGamepadButton_LeftBumper] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
	        gamepad_state->button_current[peGamepadButton_RightBumper] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
	        gamepad_state->button_current[peGamepadButton_Select] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
	        gamepad_state->button_current[peGamepadButton_Start] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_START];
	        gamepad_state->button_current[peGamepadButton_Guide] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE];
	        gamepad_state->button_current[peGamepadButton_LeftThumb] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
	        gamepad_state->button_current[peGamepadButton_RightThumb] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
	        gamepad_state->button_current[peGamepadButton_DpadDown] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
	        gamepad_state->button_current[peGamepadButton_DpadRight] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
	        gamepad_state->button_current[peGamepadButton_DpadLeft] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
	        gamepad_state->button_current[peGamepadButton_DpadUp] |= glfw_gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];

            gamepad_state->axis[peGamepadAxis_LeftX] += glfw_gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
	        gamepad_state->axis[peGamepadAxis_LeftY] += glfw_gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
	        gamepad_state->axis[peGamepadAxis_RightX] += glfw_gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
	        gamepad_state->axis[peGamepadAxis_RightY] += glfw_gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
	        gamepad_state->axis[peGamepadAxis_LeftTrigger] += glfw_gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
	        gamepad_state->axis[peGamepadAxis_RightTrigger] += glfw_gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
        }

        for (int a = 0; a < peGamepadAxis_Count; a += 1) {
            gamepad_state->axis[a] = P_CLAMP(gamepad_state->axis[a], -1.0f, 1.0f);
        }
    }
}

static int pe_input_key_map_glfw(peKeyboardKey key) {
    switch (key) {
	    case peKeyboardKey_Down: return GLFW_KEY_S;
	    case peKeyboardKey_Right: return GLFW_KEY_D;
	    case peKeyboardKey_Left: return GLFW_KEY_A;
	    case peKeyboardKey_Up: return GLFW_KEY_W;
        case peKeyboardKey_O: return GLFW_KEY_O;
        default: break;
    }
    return 0;
}

bool pe_input_key_is_down_glfw(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = pe_input_key_map_glfw(key);
    return pe_input_state_glfw.keyboard_state.current.key_down[key_glfw];
}

bool pe_input_key_was_down_glfw(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = pe_input_key_map_glfw(key);
    return !pe_input_state_glfw.keyboard_state.current.key_down[key_glfw];
}

bool pe_input_key_pressed_glfw(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = pe_input_key_map_glfw(key);
    bool key_down = pe_input_state_glfw.keyboard_state.current.key_down[key_glfw];
    int key_state_changes = pe_input_state_glfw.keyboard_state.current.key_state_changes[key_glfw];
    return (key_down && key_state_changes > 0) || (key_state_changes >= 2);
}

bool pe_input_key_released_glfw(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = pe_input_key_map_glfw(key);
    bool key_down = pe_input_state_glfw.keyboard_state.current.key_down[key_glfw];
    int key_state_changes = pe_input_state_glfw.keyboard_state.current.key_state_changes[key_glfw];
    return (!key_down && key_state_changes > 0) || (key_state_changes >= 2);
}

void pe_input_mouse_positon_glfw(float *pos_x, float *pos_y) {
    *pos_x = (float)pe_input_state_glfw.mouse_state.pos_x;
    *pos_y = (float)pe_input_state_glfw.mouse_state.pos_y;
}

bool pe_input_gamepad_is_down_glfw(peGamepadButton button) {
    return pe_input_state_glfw.gamepad_state.button_current[button];
}

bool pe_input_gamepad_was_down_glfw(peGamepadButton button) {
    return pe_input_state_glfw.gamepad_state.button_last[button];
}

float pe_input_gamepad_axis_glfw(peGamepadAxis axis) {
    return pe_input_state_glfw.gamepad_state.axis[axis];
}