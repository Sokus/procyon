#ifndef PE_CLIENT_H_HEADER_GUARD
#define PE_CLIENT_H_HEADER_GUARD

#include <stdbool.h>

struct peSocket;
void pe_client_init(struct peSocket *socket);
void pe_client_update(void);
bool pe_client_should_quit(void);
void pe_client_shutdown(void);


#endif // PE_CLIENT_H_HEADER_GUARD