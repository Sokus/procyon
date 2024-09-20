#include "p_input.h"

#include "core/p_defines.h"
#include "core/p_assert.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <stdbool.h>
#include <string.h>

#define P_KEY_COUNT_GLFW (GLFW_KEY_LAST+1)

// TODO:
// - static assert for GLFW button/axis count

typedef struct pInputGLFW {
    struct {
        bool button[P_KEY_COUNT_GLFW];
        int state_changes[P_KEY_COUNT_GLFW];
    } keyboard;
    struct {
        double x;
        double y;
    } mouse;
    struct {
        bool button[peGamepadButton_Count];
        float axis[peGamepadAxis_Count];
    } gamepad;
} pInputGLFW;

struct pInputStateGLFW {
    pInputGLFW input[2];
    int input_current;
} p_input_state_glfw = {0};

static peGamepadButton p_input_gamepad_button_map(int button_glfw);
static peGamepadAxis p_input_gamepad_axis_map(int axis_glfw);
static int p_input_key_map_glfw(peKeyboardKey key);

void pe_input_init(void) {

}

void pe_input_update(void) {
    pInputGLFW *input_current, *input_last;
    {
        int input_last_index = p_input_state_glfw.input_current;
        int input_current_index = (input_last_index + 1) % 2;
        input_current = &p_input_state_glfw.input[input_current_index];
        input_last = &p_input_state_glfw.input[input_last_index];
        p_input_state_glfw.input_current = input_current_index;
    }

    // move/reset keyboard state
    memcpy(input_current->keyboard.button, input_last->keyboard.button, sizeof(input_current->keyboard.button));
    memset(input_current->keyboard.state_changes, 0, sizeof(input_current->keyboard.state_changes));

    // move mouse state
    memcpy(&input_current->mouse, &input_last->mouse, sizeof(input_last->mouse));

    // reset gamepad state
    memset(&input_current->gamepad, 0, sizeof(input_current->gamepad));

    // update gamepad state
    {
        for (int joystick_id = GLFW_JOYSTICK_1; joystick_id < GLFW_JOYSTICK_LAST; joystick_id += 1) {
            GLFWgamepadstate glfw_gamepad_state;
            if (GLFW_FALSE == glfwGetGamepadState(joystick_id, &glfw_gamepad_state)) {
                continue;
            }
            for (int button_glfw = 0; button_glfw <= GLFW_GAMEPAD_BUTTON_LAST; button_glfw += 1) {
                peGamepadButton button = p_input_gamepad_button_map(button_glfw);
                input_current->gamepad.button[button] |= glfw_gamepad_state.buttons[button_glfw];
            }
            for (int axis_glfw = 0; axis_glfw <= GLFW_GAMEPAD_AXIS_LAST; axis_glfw += 1) {
                peGamepadAxis axis = p_input_gamepad_axis_map(axis_glfw);
                input_current->gamepad.axis[axis] += glfw_gamepad_state.axes[axis_glfw];
            }
        }

        for (int axis = 0; axis < peGamepadAxis_Count; axis += 1) {
            input_current->gamepad.axis[axis] = P_CLAMP(input_current->gamepad.axis[axis], -1.0f, 1.0f);
        }
    }
}

bool pe_input_key_is_down(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = p_input_key_map_glfw(key);
    int input_current = p_input_state_glfw.input_current;
    return p_input_state_glfw.input[input_current].keyboard.button[key_glfw];
}

bool pe_input_key_was_down(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = p_input_key_map_glfw(key);
    int input_current = p_input_state_glfw.input_current;
    int input_last = (p_input_state_glfw.input_current + 1) % 2;
    bool ended_down_last_frame = p_input_state_glfw.input[input_last].keyboard.button[key_glfw];
    bool was_down_this_frame = (
        !p_input_state_glfw.input[input_current].keyboard.button[key_glfw] &&
        p_input_state_glfw.input[input_current].keyboard.state_changes[key_glfw] > 0
    );
    return ended_down_last_frame || was_down_this_frame;
}

bool pe_input_key_pressed(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = p_input_key_map_glfw(key);
    int input_current = p_input_state_glfw.input_current;
    bool key_down = p_input_state_glfw.input[input_current].keyboard.button[key_glfw];
    int key_state_changes = p_input_state_glfw.input[input_current].keyboard.state_changes[key_glfw];
    return (key_down && key_state_changes > 0) || (key_state_changes >= 2);
}

bool pe_input_key_released(peKeyboardKey key) {
    P_ASSERT(key >= 0 && key < peKeyboardKey_Count);
    int key_glfw = p_input_key_map_glfw(key);
    int input_current = p_input_state_glfw.input_current;
    bool key_down = p_input_state_glfw.input[input_current].keyboard.button[key_glfw];
    int key_state_changes = p_input_state_glfw.input[input_current].keyboard.state_changes[key_glfw];
    return (!key_down && key_state_changes > 0) || (key_state_changes >= 2);
}

void pe_input_mouse_positon(float *pos_x, float *pos_y) {
    int input_current = p_input_state_glfw.input_current;
    *pos_x = (float)p_input_state_glfw.input[input_current].mouse.x;
    *pos_y = (float)p_input_state_glfw.input[input_current].mouse.y;
}

bool pe_input_gamepad_is_down(peGamepadButton button) {
    P_ASSERT(button >= 0 && button < peGamepadButton_Count);
    int input_current = p_input_state_glfw.input_current;
    return p_input_state_glfw.input[input_current].gamepad.button[button];
}

bool pe_input_gamepad_was_down(peGamepadButton button) {
    P_ASSERT(button >= 0 && button < peGamepadButton_Count);
    int input_last = (p_input_state_glfw.input_current + 1) % 2;
    return p_input_state_glfw.input[input_last].gamepad.button[button];
}

float pe_input_gamepad_axis(peGamepadAxis axis) {
    P_ASSERT(axis >= 0 && axis < peGamepadAxis_Count);
    int input_current = p_input_state_glfw.input_current;
    return p_input_state_glfw.input[input_current].gamepad.axis[axis];
}

void pe_input_key_callback(int key, int action) {
    if (action != GLFW_PRESS && action != GLFW_RELEASE) {
        return;
    }
    pInputGLFW *input_current = &p_input_state_glfw.input[p_input_state_glfw.input_current];
    // TODO: check if it crashes if we launch the game with key pressed
    bool button_down = (action == GLFW_PRESS);
    P_ASSERT(button_down != input_current->keyboard.button[key]);
    input_current->keyboard.button[key] = button_down;
    input_current->keyboard.state_changes[key] += 1;
}

void pe_input_cursor_position_callback(double pos_x, double pos_y) {
    int input_current = p_input_state_glfw.input_current;
    p_input_state_glfw.input[input_current].mouse.x = pos_x;
    p_input_state_glfw.input[input_current].mouse.y = pos_y;
}

////////////////////////////////////////////////////////////////////////////////

static peGamepadButton p_input_gamepad_button_map(int button_glfw) {
    peGamepadButton result = 0;
    switch (button_glfw) {
        case GLFW_GAMEPAD_BUTTON_A: result = peGamepadButton_ActionDown; break;
        case GLFW_GAMEPAD_BUTTON_B: result = peGamepadButton_ActionRight; break;
        case GLFW_GAMEPAD_BUTTON_X: result = peGamepadButton_ActionLeft; break;
        case GLFW_GAMEPAD_BUTTON_Y: result = peGamepadButton_ActionUp; break;
        case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: result = peGamepadButton_LeftBumper; break;
        case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: result = peGamepadButton_RightBumper; break;
        case GLFW_GAMEPAD_BUTTON_BACK: result = peGamepadButton_Select; break;
        case GLFW_GAMEPAD_BUTTON_START: result = peGamepadButton_Start; break;
        case GLFW_GAMEPAD_BUTTON_GUIDE: result = peGamepadButton_Guide; break;
        case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: result = peGamepadButton_LeftThumb; break;
        case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: result = peGamepadButton_RightThumb; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: result = peGamepadButton_DpadDown; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: result = peGamepadButton_DpadRight; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: result = peGamepadButton_DpadLeft; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_UP: result = peGamepadButton_DpadUp; break;
        default: break;
    }
    return result;
}

static peGamepadAxis p_input_gamepad_axis_map(int axis_glfw) {
    peGamepadAxis result = 0;
    switch (axis_glfw) {
        case GLFW_GAMEPAD_AXIS_LEFT_X: result = peGamepadAxis_LeftX; break;
        case GLFW_GAMEPAD_AXIS_LEFT_Y: result = peGamepadAxis_LeftY; break;
        case GLFW_GAMEPAD_AXIS_RIGHT_X: result = peGamepadAxis_RightX; break;
        case GLFW_GAMEPAD_AXIS_RIGHT_Y: result = peGamepadAxis_RightY; break;
        case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER: result = peGamepadAxis_LeftTrigger; break;
        case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: result = peGamepadAxis_RightTrigger; break;
        default: break;
    }
    return result;
}

// FIXME
static int p_input_key_map_glfw(peKeyboardKey key) {
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
