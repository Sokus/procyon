#ifndef P_INPUT_HEADER_GUARD
#define P_INPUT_HEADER_GUARD

#include <stdbool.h>

// FIXME
typedef enum pKeyboardKey {
	pKeyboardKey_Down,
	pKeyboardKey_Right,
	pKeyboardKey_Left,
	pKeyboardKey_Up,
	pKeyboardKey_O, // temporary
	pKeyboardKey_Count,
} pKeyboardKey;

typedef enum pGamepadButton {
	pGamepadButton_ActionDown,
	pGamepadButton_ActionRight,
	pGamepadButton_ActionLeft,
	pGamepadButton_ActionUp,
	pGamepadButton_LeftBumper,
	pGamepadButton_RightBumper,
	pGamepadButton_Select,
	pGamepadButton_Start,
	pGamepadButton_Guide,
	pGamepadButton_LeftThumb,
	pGamepadButton_RightThumb,
	pGamepadButton_DpadDown,
	pGamepadButton_DpadRight,
	pGamepadButton_DpadLeft,
	pGamepadButton_DpadUp,
	pGamepadButton_Count,
} pGamepadButton;

typedef enum pGamepadAxis {
	pGamepadAxis_LeftX,
	pGamepadAxis_LeftY,
	pGamepadAxis_RightX,
	pGamepadAxis_RightY,
	pGamepadAxis_LeftTrigger,
	pGamepadAxis_RightTrigger,
	pGamepadAxis_Count,
} pGamepadAxis;


void p_input_init(void);
void p_input_update(void);

bool p_input_key_is_down(pKeyboardKey key);
bool p_input_key_was_down(pKeyboardKey key);
bool p_input_key_pressed(pKeyboardKey key);
bool p_input_key_released(pKeyboardKey key);

void p_input_mouse_positon(float *pos_x, float *pos_y);

bool p_input_gamepad_is_down(pGamepadButton button);
bool p_input_gamepad_was_down(pGamepadButton button);
bool p_input_gamepad_pressed(pGamepadButton button);
bool p_input_gamepad_released(pGamepadButton button);
float p_input_gamepad_axis(pGamepadAxis axis);

void p_input_key_callback(int key, int action);
void p_input_cursor_position_callback(double pos_x, double pos_y);

#endif // P_INPUT_HEADER_GUARD