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
        bool button[pGamepadButton_Count];
        float axis[pGamepadAxis_Count];
    } gamepad;
} pInputGLFW;

struct pInputStateGLFW {
    pInputGLFW input[2];
    int input_current;
} p_input_state_glfw = {0};

static pGamepadButton p_input_gamepad_button_map(int button_glfw);
static pGamepadAxis p_input_gamepad_axis_map(int axis_glfw);
static int p_input_key_map_glfw(pKeyboardKey key);

void p_input_init(void) {

}

void p_input_update(void) {
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
                pGamepadButton button = p_input_gamepad_button_map(button_glfw);
                input_current->gamepad.button[button] |= glfw_gamepad_state.buttons[button_glfw];
            }
            for (int axis_glfw = 0; axis_glfw <= GLFW_GAMEPAD_AXIS_LAST; axis_glfw += 1) {
                pGamepadAxis axis = p_input_gamepad_axis_map(axis_glfw);
                input_current->gamepad.axis[axis] += glfw_gamepad_state.axes[axis_glfw];
            }
        }

        for (int axis = 0; axis < pGamepadAxis_Count; axis += 1) {
            input_current->gamepad.axis[axis] = P_CLAMP(input_current->gamepad.axis[axis], -1.0f, 1.0f);
        }
    }
}

bool p_input_key_is_down(pKeyboardKey key) {
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
    int key_glfw = p_input_key_map_glfw(key);
    int input_current = p_input_state_glfw.input_current;
    return p_input_state_glfw.input[input_current].keyboard.button[key_glfw];
}

bool p_input_key_was_down(pKeyboardKey key) {
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
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

bool p_input_key_pressed(pKeyboardKey key) {
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
    int key_glfw = p_input_key_map_glfw(key);
    int input_current = p_input_state_glfw.input_current;
    bool key_down = p_input_state_glfw.input[input_current].keyboard.button[key_glfw];
    int key_state_changes = p_input_state_glfw.input[input_current].keyboard.state_changes[key_glfw];
    return (key_down && key_state_changes > 0) || (key_state_changes >= 2);
}

bool p_input_key_released(pKeyboardKey key) {
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
    int key_glfw = p_input_key_map_glfw(key);
    int input_current = p_input_state_glfw.input_current;
    bool key_down = p_input_state_glfw.input[input_current].keyboard.button[key_glfw];
    int key_state_changes = p_input_state_glfw.input[input_current].keyboard.state_changes[key_glfw];
    return (!key_down && key_state_changes > 0) || (key_state_changes >= 2);
}

void p_input_mouse_positon(float *pos_x, float *pos_y) {
    int input_current = p_input_state_glfw.input_current;
    *pos_x = (float)p_input_state_glfw.input[input_current].mouse.x;
    *pos_y = (float)p_input_state_glfw.input[input_current].mouse.y;
}

bool p_input_gamepad_is_down(pGamepadButton button) {
    P_ASSERT(button >= 0 && button < pGamepadButton_Count);
    int input_current = p_input_state_glfw.input_current;
    return p_input_state_glfw.input[input_current].gamepad.button[button];
}

bool p_input_gamepad_was_down(pGamepadButton button) {
    P_ASSERT(button >= 0 && button < pGamepadButton_Count);
    int input_last = (p_input_state_glfw.input_current + 1) % 2;
    return p_input_state_glfw.input[input_last].gamepad.button[button];
}

float p_input_gamepad_axis(pGamepadAxis axis) {
    P_ASSERT(axis >= 0 && axis < pGamepadAxis_Count);
    int input_current = p_input_state_glfw.input_current;
    return p_input_state_glfw.input[input_current].gamepad.axis[axis];
}

void p_input_key_callback(int key, int action) {
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

void p_input_cursor_position_callback(double pos_x, double pos_y) {
    int input_current = p_input_state_glfw.input_current;
    p_input_state_glfw.input[input_current].mouse.x = pos_x;
    p_input_state_glfw.input[input_current].mouse.y = pos_y;
}

////////////////////////////////////////////////////////////////////////////////

static pGamepadButton p_input_gamepad_button_map(int button_glfw) {
    pGamepadButton result = 0;
    switch (button_glfw) {
        case GLFW_GAMEPAD_BUTTON_A: result = pGamepadButton_ActionDown; break;
        case GLFW_GAMEPAD_BUTTON_B: result = pGamepadButton_ActionRight; break;
        case GLFW_GAMEPAD_BUTTON_X: result = pGamepadButton_ActionLeft; break;
        case GLFW_GAMEPAD_BUTTON_Y: result = pGamepadButton_ActionUp; break;
        case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: result = pGamepadButton_LeftBumper; break;
        case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: result = pGamepadButton_RightBumper; break;
        case GLFW_GAMEPAD_BUTTON_BACK: result = pGamepadButton_Select; break;
        case GLFW_GAMEPAD_BUTTON_START: result = pGamepadButton_Start; break;
        case GLFW_GAMEPAD_BUTTON_GUIDE: result = pGamepadButton_Guide; break;
        case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: result = pGamepadButton_LeftThumb; break;
        case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: result = pGamepadButton_RightThumb; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: result = pGamepadButton_DpadDown; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: result = pGamepadButton_DpadRight; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: result = pGamepadButton_DpadLeft; break;
        case GLFW_GAMEPAD_BUTTON_DPAD_UP: result = pGamepadButton_DpadUp; break;
        default: break;
    }
    return result;
}

static pGamepadAxis p_input_gamepad_axis_map(int axis_glfw) {
    pGamepadAxis result = 0;
    switch (axis_glfw) {
        case GLFW_GAMEPAD_AXIS_LEFT_X: result = pGamepadAxis_LeftX; break;
        case GLFW_GAMEPAD_AXIS_LEFT_Y: result = pGamepadAxis_LeftY; break;
        case GLFW_GAMEPAD_AXIS_RIGHT_X: result = pGamepadAxis_RightX; break;
        case GLFW_GAMEPAD_AXIS_RIGHT_Y: result = pGamepadAxis_RightY; break;
        case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER: result = pGamepadAxis_LeftTrigger; break;
        case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: result = pGamepadAxis_RightTrigger; break;
        default: break;
    }
    return result;
}

// FIXME
static int p_input_key_map_glfw(pKeyboardKey key) {
    switch (key) {
	    case pKeyboardKey_Down: return GLFW_KEY_S;
	    case pKeyboardKey_Right: return GLFW_KEY_D;
	    case pKeyboardKey_Left: return GLFW_KEY_A;
	    case pKeyboardKey_Up: return GLFW_KEY_W;
        case pKeyboardKey_O: return GLFW_KEY_O;
        default: break;
    }
    return 0;
}
