#include "pe_input_psp.h"
#include "pe_core.h"

#include <pspctrl.h>

struct peInputStatePSP {
    bool button_current[peGamepadButton_Count];
    bool button_last[peGamepadButton_Count];
    float left_axis[2];
} pe_input_state_psp = {0};

void pe_input_init_psp(void) {
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

void pe_input_update_psp(void) {
    SceCtrlData ctrl_data;
    sceCtrlPeekBufferPositive(&ctrl_data, 1);
	pe_input_state_psp.button_current[peGamepadButton_ActionDown] = ctrl_data.Buttons & PSP_CTRL_CROSS;
    pe_input_state_psp.button_current[peGamepadButton_ActionRight] = ctrl_data.Buttons & PSP_CTRL_CIRCLE;
    pe_input_state_psp.button_current[peGamepadButton_ActionLeft] = ctrl_data.Buttons & PSP_CTRL_SQUARE;
    pe_input_state_psp.button_current[peGamepadButton_ActionUp] = ctrl_data.Buttons & PSP_CTRL_TRIANGLE;
    pe_input_state_psp.button_current[peGamepadButton_LeftBumper] = ctrl_data.Buttons & PSP_CTRL_LTRIGGER;
    pe_input_state_psp.button_current[peGamepadButton_RightBumper] = ctrl_data.Buttons & PSP_CTRL_RTRIGGER;
    pe_input_state_psp.button_current[peGamepadButton_Select] = ctrl_data.Buttons & PSP_CTRL_SELECT;
    pe_input_state_psp.button_current[peGamepadButton_Start] = ctrl_data.Buttons & PSP_CTRL_START;
    pe_input_state_psp.button_current[peGamepadButton_DpadDown] = ctrl_data.Buttons & PSP_CTRL_DOWN;
    pe_input_state_psp.button_current[peGamepadButton_DpadLeft] = ctrl_data.Buttons & PSP_CTRL_LEFT;
    pe_input_state_psp.button_current[peGamepadButton_DpadRight] = ctrl_data.Buttons & PSP_CTRL_RIGHT;
    pe_input_state_psp.button_current[peGamepadButton_DpadUp] = ctrl_data.Buttons & PSP_CTRL_UP;

    // TODO: Check out sceCtrlSetIdleCancelThreshold(), will this let us specify
    // a deadzone for the analog?
    const float deadzone_x = 0.20f;
    const float deadzone_y = 0.15f;
    pe_input_state_psp.left_axis[0] = pe_psp_process_axis_value(ctrl_data.Lx, deadzone_x);
    pe_input_state_psp.left_axis[1] = pe_psp_process_axis_value(ctrl_data.Ly, deadzone_y);
}

bool pe_input_gamepad_is_down_psp(peGamepadButton button) {
    return pe_input_state_psp.button_current[button];
}

bool pe_input_gamepad_was_down_psp(peGamepadButton button) {
    return pe_input_state_psp.button_last[button];
}

float pe_input_gamepad_axis_psp(peGamepadAxis axis) {
    switch (axis) {
        case peGamepadAxis_LeftX: return pe_input_state_psp.left_axis[0];
        case peGamepadAxis_LeftY: return pe_input_state_psp.left_axis[1];
        default: break;
    }
    return 0.0f;
}