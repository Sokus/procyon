#ifndef PE_CLIENT_H_HEADER_GUARD
#define PE_CLIENT_H_HEADER_GUARD

#include <stdbool.h>

struct peSocket;
struct peArena;
struct peMessage;
struct peAddress;
void pe_client_init(struct peArena *temp_arena, struct peSocket *socket);
void pe_client_update(struct peArena *temp_arena);
bool pe_client_should_quit(void);
void pe_client_shutdown(void);
void pe_client_process_message(struct peMessage message, struct peAddress sender);


#endif // PE_CLIENT_H_HEADER_GUARD