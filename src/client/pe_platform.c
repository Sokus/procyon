#include "pe_platform.h"

#include <stdbool.h>

#if defined(_WIN32)
    #include "pe_window_glfw.h"
#elif defined(PSP)
    #include "pe_platform_psp.h"
#endif

static bool platform_initialized = false;

void pe_platform_init(void) {
    bool success = true;
#if defined(PSP)
    success &= pe_platform_init_psp();
#endif
    platform_initialized = success;
}

bool pe_platform_should_quit(void) {
#if defined(_WIN32)
    return pe_window_should_quit_glfw();
#elif defined(PSP)
    return pe_platform_should_quit_psp();
#endif
}

void pe_platform_poll_events(void) {
#if defined(_WIN32)
    pe_window_poll_events_glfw();
#endif
}
