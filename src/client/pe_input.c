#include "pe_input.h"
#include "pe_core.h"

#include <stdbool.h>

#if defined(_WIN32) || defined(__linux__)
    #include "pe_input_glfw.h"
#elif defined(PSP)
    #include "pe_input_psp.h"
#endif

static bool input_initialized = false;

struct peInputState {
	bool curr_buttons[peGamepadButton_Count];
	bool prev_buttons[peGamepadButton_Count];
	float axis[peGamepadAxis_Count];
} input_state = {0};

void pe_input_init(void) {
#if defined(PSP)
    pe_input_init_psp();
#endif
    input_initialized = true;
}

void pe_input_update(void) {
    PE_ASSERT(input_initialized);
#if defined(_WIN32) || defined(__linux__)
    pe_input_update_glfw();
#elif defined(PSP)
    pe_input_update_psp();
#endif
}

bool pe_input_key_is_down(peKeyboardKey key) {
    PE_ASSERT(key >= 0 && key < peKeyboardKey_Count);
#if defined(_WIN32) || defined(__linux__)
    return pe_input_key_is_down_glfw(key);
#else
    return false;
#endif
}

bool pe_input_key_was_down(peKeyboardKey key) {
    PE_ASSERT(key >= 0 && key < peKeyboardKey_Count);
#if defined(_WIN32) || defined(__linux__)
    return pe_input_key_was_down_glfw(key);
#else
    return false;
#endif
}

bool pe_input_key_pressed(peKeyboardKey key) {
    PE_ASSERT(key >= 0 && key < peKeyboardKey_Count);
#if defined(_WIN32) || defined(__linux__)
    return pe_input_key_pressed_glfw(key);
#else
    return false;
#endif
}

bool pe_input_key_released(peKeyboardKey key) {
    PE_ASSERT(key >= 0 && key < peKeyboardKey_Count);
#if defined(_WIN32) || defined(__linux__)
    return pe_input_key_released_glfw(key);
#else
    return false;
#endif
}

void pe_input_mouse_positon(float *pos_x, float *pos_y) {
#if defined(_WIN32) || defined(__linux__)
    pe_input_mouse_positon_glfw(pos_x, pos_y);
#elif defined(PSP)
    *pos_x = 0.5f;
    *pos_y = 0.5f;
#endif
}

bool pe_input_gamepad_is_down(peGamepadButton button) {
    PE_ASSERT(button >= 0 && button < peGamepadButton_Count);
#if defined(_WIN32) || defined(__linux__)
    return pe_input_gamepad_is_down_glfw(button);
#elif defined(PSP)
    return pe_input_gamepad_is_down_psp(button);
#endif
}

bool pe_input_gamepad_was_down(peGamepadButton button) {
    PE_ASSERT(button >= 0 && button < peGamepadButton_Count);
#if defined(_WIN32) || defined(__linux__)
    return pe_input_gamepad_was_down_glfw(button);
#elif defined(PSP)
    return pe_input_gamepad_was_down_psp(button);
#endif
}

bool pe_input_gamepad_pressed(peGamepadButton button) {
    bool is_down = pe_input_gamepad_is_down(button);
    bool was_down = pe_input_gamepad_was_down(button);
    return is_down && !was_down;
}

bool pe_input_gamepad_released(peGamepadButton button) {
    bool is_down = pe_input_gamepad_is_down(button);
    bool was_down = pe_input_gamepad_was_down(button);
    return !is_down && was_down;
}

float pe_input_gamepad_axis(peGamepadAxis axis) {
#if defined(_WIN32) || defined(__linux__)
    return pe_input_gamepad_axis_glfw(axis);
#elif defined(PSP)
    return pe_input_gamepad_axis_psp(axis);
#endif
}