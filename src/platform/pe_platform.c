#include "pe_platform.h"
#include "core/p_assert.h"

#include <stdbool.h>

#if defined(PSP)
    #include "pe_platform_psp.h"
#endif

static bool platform_initialized = false;

void pe_platform_init(void) {
    P_ASSERT(!platform_initialized);
    bool success = true;
#if defined(PSP)
    success &= pe_platform_init_psp();
#endif
    platform_initialized = success;
}

void pe_platform_shutdown(void) {
    P_ASSERT(platform_initialized);
#if defined(PSP)
    pe_platform_shutdown_psp();
#endif
    platform_initialized = false;
}

bool pe_platform_should_quit(void) {
    P_ASSERT(platform_initialized);
    bool result = false;
#if defined(PSP)
    result = pe_platform_should_quit_psp();
#endif
    return result;
}