#include "p_input.h"

#include <stdbool.h>

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
