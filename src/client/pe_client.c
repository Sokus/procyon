#include "pe_client.h"

#include "pe_net.h"
#include "pe_platform.h"
#include "pe_graphics.h"

#include <stdbool.h>

struct peClientState {
    peSocket *socket;

    peCamera camera;
} client_state = {0};

void pe_client_init(peSocket *socket) {
    client_state.socket = socket;

    pe_platform_init();
    pe_graphics_init(960, 540, "Procyon");
}

void pe_client_update(void) {
    pe_platform_poll_events();

    pe_graphics_frame_begin();
    pe_clear_background((peColor){ 20, 20, 20, 255 });
    pe_graphics_frame_end(true);
}

bool pe_client_should_quit(void) {
    return pe_platform_should_quit();
}

void pe_client_shutdown(void) {
    pe_graphics_shutdown();
    pe_platform_shutdown();
}
