#ifndef PE_INPUT_PSP_H_HEADER_GUARD
#define PE_INPUT_PSP_H_HEADER_GUARD

#include "pe_input.h"

void pe_input_init_psp(void);
void pe_input_update_psp(void);

bool pe_input_gamepad_is_down_psp(peGamepadButton button);
bool pe_input_gamepad_was_down_psp(peGamepadButton button);
float pe_input_gamepad_axis_psp(peGamepadAxis axis);


#endif // PE_INPUT_PSP_H_HEADER_GUARD