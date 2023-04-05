#include "pe_input.h"
#include "core.h"

#include <stdbool.h>
#include <string.h>
#include <pspctrl.h>

peGamepad gamepad = {0};
static bool input_initialized = false;

void pe_input_init(void) {
    input_initialized = true;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

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

void pe_input_update(void) {
    PE_ASSERT(input_initialized);

	//memcpy(gamepad.prev_buttons, gamepad.curr_buttons, sizeof(gamepad.curr_buttons));

	SceCtrlData ctrl_data;
	sceCtrlReadBufferPositive(&ctrl_data, 1);

	gamepad.curr_buttons[peGamepadButton_ActionDown] = ctrl_data.Buttons & PSP_CTRL_CROSS;
    gamepad.curr_buttons[peGamepadButton_ActionRight] = ctrl_data.Buttons & PSP_CTRL_CIRCLE;
    gamepad.curr_buttons[peGamepadButton_ActionLeft] = ctrl_data.Buttons & PSP_CTRL_SQUARE;
    gamepad.curr_buttons[peGamepadButton_ActionUp] = ctrl_data.Buttons & PSP_CTRL_TRIANGLE;
    gamepad.curr_buttons[peGamepadButton_LeftBumper] = ctrl_data.Buttons & PSP_CTRL_LTRIGGER;
    gamepad.curr_buttons[peGamepadButton_RightBumper] = ctrl_data.Buttons & PSP_CTRL_RTRIGGER;
    gamepad.curr_buttons[peGamepadButton_Select] = ctrl_data.Buttons & PSP_CTRL_SELECT;
    gamepad.curr_buttons[peGamepadButton_Start] = ctrl_data.Buttons & PSP_CTRL_START;
    gamepad.curr_buttons[peGamepadButton_DpadDown] = ctrl_data.Buttons & PSP_CTRL_DOWN;
    gamepad.curr_buttons[peGamepadButton_DpadLeft] = ctrl_data.Buttons & PSP_CTRL_LEFT;
    gamepad.curr_buttons[peGamepadButton_DpadRight] = ctrl_data.Buttons & PSP_CTRL_RIGHT;
    gamepad.curr_buttons[peGamepadButton_DpadUp] = ctrl_data.Buttons & PSP_CTRL_UP;

    gamepad.axis[peGamepadAxis_LeftX] = pe_psp_process_axis_value(ctrl_data.Lx, 0.2f);
    gamepad.axis[peGamepadAxis_LeftY] = pe_psp_process_axis_value(ctrl_data.Ly, 0.15f);
}

float pe_input_axis(peGamepadAxis axis) {
    return gamepad.axis[axis];
}