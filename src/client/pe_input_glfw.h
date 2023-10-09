#ifndef PE_INPUT_GLFW_H_HEADER_GUARD
#define PE_INPUT_GLFW_H_HEADER_GUARD

#include "pe_input.h"

void pe_input_update_glfw(void);
void pe_input_key_callback_glfw(int key, int action);
void pe_input_cursor_position_callback_glfw(double pos_x, double pos_y);

bool pe_input_key_is_down_glfw(peKeyboardKey key);
bool pe_input_key_was_down_glfw(peKeyboardKey key);
bool pe_input_key_pressed_glfw(peKeyboardKey key);
bool pe_input_key_released_glfw(peKeyboardKey key);

void pe_input_mouse_positon_glfw(float *pos_x, float *pos_y);

bool pe_input_gamepad_is_down_glfw(peGamepadButton button);
bool pe_input_gamepad_was_down_glfw(peGamepadButton button);
float pe_input_gamepad_axis_glfw(peGamepadAxis axis);

#endif // PE_INPUT_GLFW_H_HEADER_GUARD