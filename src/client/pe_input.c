#include "pe_input.h"
#include "pe_core.h"

#include <stdbool.h>

#if defined(_WIN32) || defined(__linux__)
    #define GLFW_INCLUDE_NONE
    #include "GLFW/glfw3.h"
#elif defined(PSP)
    #include <pspctrl.h>
#endif

static bool input_initialized = false;

struct peInputState {
	bool curr_buttons[peGamepadButton_Count];
	bool prev_buttons[peGamepadButton_Count];
	float axis[peGamepadAxis_Count];
} input_state = {0};

void pe_input_init(void) {
    input_initialized = true;
#if defined(PSP)
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
#endif
}

#if defined(_WIN32) || defined(__linux__)
static void pe_input_update_pc(void) {
    bool any_joystick_connected = false;
    int first_connected_joystick_id;
    for (int joystick_id = GLFW_JOYSTICK_1; joystick_id < GLFW_JOYSTICK_LAST; joystick_id += 1) {
        if (glfwJoystickPresent(joystick_id) == GLFW_TRUE) {
            any_joystick_connected = true;
            first_connected_joystick_id = joystick_id;
        }
    }

    if (any_joystick_connected) {
        GLFWgamepadstate gamepad_state;
        if (glfwGetGamepadState(first_connected_joystick_id, &gamepad_state)) {
            input_state.curr_buttons[peGamepadButton_ActionDown] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_A];
	        input_state.curr_buttons[peGamepadButton_ActionRight] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_B];
	        input_state.curr_buttons[peGamepadButton_ActionLeft] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_X];
	        input_state.curr_buttons[peGamepadButton_ActionUp] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_Y];
	        input_state.curr_buttons[peGamepadButton_LeftBumper] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
	        input_state.curr_buttons[peGamepadButton_RightBumper] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
	        input_state.curr_buttons[peGamepadButton_Select] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
	        input_state.curr_buttons[peGamepadButton_Start] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_START];
	        input_state.curr_buttons[peGamepadButton_Guide] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE];
	        input_state.curr_buttons[peGamepadButton_LeftThumb] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
	        input_state.curr_buttons[peGamepadButton_RightThumb] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
	        input_state.curr_buttons[peGamepadButton_DpadDown] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
	        input_state.curr_buttons[peGamepadButton_DpadRight] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
	        input_state.curr_buttons[peGamepadButton_DpadLeft] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
	        input_state.curr_buttons[peGamepadButton_DpadUp] = gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];

            input_state.axis[peGamepadAxis_LeftX] = gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
	        input_state.axis[peGamepadAxis_LeftY] = gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
	        input_state.axis[peGamepadAxis_RightX] = gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
	        input_state.axis[peGamepadAxis_RightY] = gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
	        input_state.axis[peGamepadAxis_LeftTrigger] = gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
	        input_state.axis[peGamepadAxis_RightTrigger] = gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
        }
    }
}
#endif

#if defined(PSP)
static float pe_psp_process_axis_value(unsigned char value, float deadzone_radius) {
	PE_ASSERT(deadzone_radius < 1.0f);
	PE_ASSERT(deadzone_radius >= 0.0f);
	float fvalue = (float)value/(255.0f/2.0f) - 1.0f;
	float effective_range = 1.0 - deadzone_radius;
	float result = 0.0f;
	if (fvalue < -deadzone_radius) {
		// convert <-1.0, deadzone) range to <-1.0, 0.0)
		result = (fvalue + 1.0f) * (1.0f/effective_range) - 1.0f;
	} else if (fvalue > deadzone_radius) {
		// convert (deadzone, 1.0) range to (0.0, 1.0>
		result = (fvalue - 1.0f) * (1.0f/effective_range) + 1.0f;
	}
    return result;
}
static void pe_input_update_psp(void) {
	SceCtrlData ctrl_data;
	sceCtrlReadBufferPositive(&ctrl_data, 1);
	input_state.curr_buttons[peGamepadButton_ActionDown] = ctrl_data.Buttons & PSP_CTRL_CROSS;
    input_state.curr_buttons[peGamepadButton_ActionRight] = ctrl_data.Buttons & PSP_CTRL_CIRCLE;
    input_state.curr_buttons[peGamepadButton_ActionLeft] = ctrl_data.Buttons & PSP_CTRL_SQUARE;
    input_state.curr_buttons[peGamepadButton_ActionUp] = ctrl_data.Buttons & PSP_CTRL_TRIANGLE;
    input_state.curr_buttons[peGamepadButton_LeftBumper] = ctrl_data.Buttons & PSP_CTRL_LTRIGGER;
    input_state.curr_buttons[peGamepadButton_RightBumper] = ctrl_data.Buttons & PSP_CTRL_RTRIGGER;
    input_state.curr_buttons[peGamepadButton_Select] = ctrl_data.Buttons & PSP_CTRL_SELECT;
    input_state.curr_buttons[peGamepadButton_Start] = ctrl_data.Buttons & PSP_CTRL_START;
    input_state.curr_buttons[peGamepadButton_DpadDown] = ctrl_data.Buttons & PSP_CTRL_DOWN;
    input_state.curr_buttons[peGamepadButton_DpadLeft] = ctrl_data.Buttons & PSP_CTRL_LEFT;
    input_state.curr_buttons[peGamepadButton_DpadRight] = ctrl_data.Buttons & PSP_CTRL_RIGHT;
    input_state.curr_buttons[peGamepadButton_DpadUp] = ctrl_data.Buttons & PSP_CTRL_UP;
    input_state.axis[peGamepadAxis_LeftX] = pe_psp_process_axis_value(ctrl_data.Lx, 0.2f);
    input_state.axis[peGamepadAxis_LeftY] = pe_psp_process_axis_value(ctrl_data.Ly, 0.15f);
}
#endif

void pe_input_update(void) {
    PE_ASSERT(input_initialized);
#if defined(_WIN32) || defined(__linux__)
    pe_input_update_pc();
#elif defined(PSP)
    pe_input_update_psp();
#endif
}

float pe_input_axis(peGamepadAxis axis) {
    return input_state.axis[axis];
}