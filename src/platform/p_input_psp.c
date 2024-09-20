#include "p_input.h"

#include "core/p_assert.h"

#include <pspctrl.h>

#include <string.h>

typedef struct pInputPSP {
    bool button[pGamepadButton_Count];
    float axis[pGamepadAxis_Count];
} pInputPSP;

struct pInputStatePSP {
    bool initialized;
    pInputPSP input[2]; // double-buffer for current/last input
    int input_current;
} p_input_state_psp = {0};

static float p_psp_process_axis_value(unsigned char value, float deadzone_radius);

void p_input_init(void) {
    p_input_state_psp.initialized = true;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void p_input_update(void) {
    P_ASSERT(p_input_state_psp.initialized);
    pInputPSP *input_current;
    {
        int input_current_index = (p_input_state_psp.input_current + 1) % 2;
        input_current = &p_input_state_psp.input[input_current_index];
        p_input_state_psp.input_current = input_current_index;
    }

    SceCtrlData ctrl_data;
    sceCtrlPeekBufferPositive(&ctrl_data, 1);
	input_current->button[pGamepadButton_ActionDown] = ctrl_data.Buttons & PSP_CTRL_CROSS;
    input_current->button[pGamepadButton_ActionRight] = ctrl_data.Buttons & PSP_CTRL_CIRCLE;
    input_current->button[pGamepadButton_ActionLeft] = ctrl_data.Buttons & PSP_CTRL_SQUARE;
    input_current->button[pGamepadButton_ActionUp] = ctrl_data.Buttons & PSP_CTRL_TRIANGLE;
    input_current->button[pGamepadButton_LeftBumper] = ctrl_data.Buttons & PSP_CTRL_LTRIGGER;
    input_current->button[pGamepadButton_RightBumper] = ctrl_data.Buttons & PSP_CTRL_RTRIGGER;
    input_current->button[pGamepadButton_Select] = ctrl_data.Buttons & PSP_CTRL_SELECT;
    input_current->button[pGamepadButton_Start] = ctrl_data.Buttons & PSP_CTRL_START;
    input_current->button[pGamepadButton_DpadDown] = ctrl_data.Buttons & PSP_CTRL_DOWN;
    input_current->button[pGamepadButton_DpadLeft] = ctrl_data.Buttons & PSP_CTRL_LEFT;
    input_current->button[pGamepadButton_DpadRight] = ctrl_data.Buttons & PSP_CTRL_RIGHT;
    input_current->button[pGamepadButton_DpadUp] = ctrl_data.Buttons & PSP_CTRL_UP;

    // TODO: Check out sceCtrlSetIdleCancelThreshold(), will this let us specify
    // a deadzone for the analog?
    const float deadzone_x = 0.20f;
    const float deadzone_y = 0.15f;
    input_current->axis[pGamepadAxis_LeftX] = p_psp_process_axis_value(ctrl_data.Lx, deadzone_x);
    input_current->axis[pGamepadAxis_LeftY] = p_psp_process_axis_value(ctrl_data.Ly, deadzone_y);
}

bool p_input_key_is_down(pKeyboardKey key) {
    P_ASSERT(p_input_state_psp.initialized);
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
    return false;
}

bool p_input_key_was_down(pKeyboardKey key) {
    P_ASSERT(p_input_state_psp.initialized);
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
    return false;
}

bool p_input_key_pressed(pKeyboardKey key) {
    P_ASSERT(p_input_state_psp.initialized);
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
    return false;
}

bool p_input_key_released(pKeyboardKey key) {
    P_ASSERT(p_input_state_psp.initialized);
    P_ASSERT(key >= 0 && key < pKeyboardKey_Count);
    return false;
}

void p_input_mouse_positon(float *pos_x, float *pos_y) {
    P_ASSERT(p_input_state_psp.initialized);
    *pos_x = 0.0f;
    *pos_y = 0.0f;
}

bool p_input_gamepad_is_down(pGamepadButton button) {
    P_ASSERT(p_input_state_psp.initialized);
    P_ASSERT(button >= 0 && button < pGamepadButton_Count);
    int input_current = p_input_state_psp.input_current;
    return p_input_state_psp.input[input_current].button[button];
}

bool p_input_gamepad_was_down(pGamepadButton button) {
    P_ASSERT(p_input_state_psp.initialized);
    P_ASSERT(button >= 0 && button < pGamepadButton_Count);
    int input_last = (p_input_state_psp.input_current + 1) % 2;
    return p_input_state_psp.input[input_last].button[button];
}

float p_input_gamepad_axis(pGamepadAxis axis) {
    P_ASSERT(p_input_state_psp.initialized);
    P_ASSERT(axis >= 0 && axis < pGamepadAxis_Count);
    int input_current = p_input_state_psp.input_current;
    return p_input_state_psp.input[input_current].axis[axis];
}

////////////////////////////////////////////////////////////////////////////////

static float p_psp_process_axis_value(unsigned char value, float deadzone_radius) {
	P_ASSERT(deadzone_radius < 1.0f);
	P_ASSERT(deadzone_radius >= 0.0f);
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
