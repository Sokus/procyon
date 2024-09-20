#include "p_input.h"

#include <stdbool.h>

bool p_input_gamepad_pressed(pGamepadButton button) {
    bool is_down = p_input_gamepad_is_down(button);
    bool was_down = p_input_gamepad_was_down(button);
    return is_down && !was_down;
}

bool p_input_gamepad_released(pGamepadButton button) {
    bool is_down = p_input_gamepad_is_down(button);
    bool was_down = p_input_gamepad_was_down(button);
    return !is_down && was_down;
}
