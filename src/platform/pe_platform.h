#ifndef PE_PLATFORM_H_HEADER_GUARD
#define PE_PLATFORM_H_HEADER_GUARD

#include <stdbool.h>

void pe_platform_init(void);
void pe_platform_shutdown(void);
bool pe_platform_should_quit(void);
void pe_platform_poll_events(void);

#endif // PE_PLATFORM_HEADER_GUARD