#include "platform/p_window.h"

#warning p_window unimplemented

void p_window_platform_init(int, int, const char *) {
}

void p_window_platform_shutdown(void) {
}

bool p_window_should_quit(void) {
    return false;
}

void p_window_poll_events(void) {
}

void p_window_swap_buffers(bool) {
}
