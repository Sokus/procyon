#ifndef PE_INPUT_H
#define PE_INPUT_H

#include <stdbool.h>

#if defined(_WIN32)

#elif defined(PSP)

typedef enum peGamepadButton {
	peGamepadButton_ActionDown  = 0,
	peGamepadButton_ActionRight = 1,
	peGamepadButton_ActionLeft  = 2,
	peGamepadButton_ActionUp    = 3,
	peGamepadButton_LeftBumper  = 4,
	peGamepadButton_RightBumper = 5,
	peGamepadButton_Select      = 6,
	peGamepadButton_Start       = 7,
	peGamepadButton_Guide       = 8,
	peGamepadButton_LeftThumb   = 9,
	peGamepadButton_RightThumb  = 10,
	peGamepadButton_DpadDown    = 11,
	peGamepadButton_DpadRight   = 12,
	peGamepadButton_DpadLeft    = 13,
	peGamepadButton_DpadUp      = 14,
	peGamepadButton_Count       = 15,
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

typedef struct peGamepad {
	bool curr_buttons[peGamepadButton_Count];
	bool prev_buttons[peGamepadButton_Count];
	float axis[peGamepadAxis_Count];
} peGamepad;

void pe_input_init(void);
void pe_input_update(void);
float pe_input_axis(peGamepadAxis axis);

#endif // #elif defined(PSP)

#endif // PE_INPUT_H