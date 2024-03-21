#ifndef PE_INPUT_H
#define PE_INPUT_H

#include <stdbool.h>

typedef enum peKeyboardKey {
	peKeyboardKey_Down,
	peKeyboardKey_Right,
	peKeyboardKey_Left,
	peKeyboardKey_Up,
	peKeyboardKey_O, // temporary
	peKeyboardKey_Count,
} peKeyboardKey;

typedef enum peGamepadButton {
	peGamepadButton_ActionDown,
	peGamepadButton_ActionRight,
	peGamepadButton_ActionLeft,
	peGamepadButton_ActionUp,
	peGamepadButton_LeftBumper,
	peGamepadButton_RightBumper,
	peGamepadButton_Select,
	peGamepadButton_Start,
	peGamepadButton_Guide,
	peGamepadButton_LeftThumb,
	peGamepadButton_RightThumb,
	peGamepadButton_DpadDown,
	peGamepadButton_DpadRight,
	peGamepadButton_DpadLeft,
	peGamepadButton_DpadUp,
	peGamepadButton_Count,
} peGamepadButton;

typedef enum peGamepadAxis {
	peGamepadAxis_LeftX,
	peGamepadAxis_LeftY,
	peGamepadAxis_RightX,
	peGamepadAxis_RightY,
	peGamepadAxis_LeftTrigger,
	peGamepadAxis_RightTrigger,
	peGamepadAxis_Count,
} peGamepadAxis;


void pe_input_init(void);
void pe_input_update(void);

bool pe_input_key_is_down(peKeyboardKey key);
bool pe_input_key_was_down(peKeyboardKey key);
bool pe_input_key_pressed(peKeyboardKey key);
bool pe_input_key_released(peKeyboardKey key);

void pe_input_mouse_positon(float *pos_x, float *pos_y);

bool pe_input_gamepad_is_down(peGamepadButton button);
bool pe_input_gamepad_was_down(peGamepadButton button);
bool pe_input_gamepad_pressed(peGamepadButton button);
bool pe_input_gamepad_released(peGamepadButton button);
float pe_input_gamepad_axis(peGamepadAxis axis);

#endif // PE_INPUT_H