#include "platform/p_input.h"

#include <stdbool.h>

#warning p_input unimplemented

void p_input_init(void) {}
void p_input_update(void) {}

bool p_input_key_is_down(pKeyboardKey key) { return false; }
bool p_input_key_was_down(pKeyboardKey key) { return false; }
bool p_input_key_pressed(pKeyboardKey key) { return false; }
bool p_input_key_released(pKeyboardKey key) { return false; }

void p_input_mouse_positon(float */*pos_x*/, float */*pos_y*/) {}

bool p_input_gamepad_is_down(pGamepadButton button) { return false; }
bool p_input_gamepad_was_down(pGamepadButton button) { return false; }
float p_input_gamepad_axis(pGamepadAxis axis) { return 0.0f; }
