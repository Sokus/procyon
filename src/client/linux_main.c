#include "pe_core.h"

#include "pe_graphics.h"
#include "pe_platform.h"

int main(int, char*[]) {
    pe_platform_init();
    pe_graphics_init(960, 540, "Procyon");

    while (!pe_platform_should_quit()) {
        pe_platform_poll_events();

        pe_graphics_frame_begin();
        pe_clear_background(PE_COLOR_RED);


        pe_graphics_frame_end(true);
    }

    pe_graphics_shutdown();


    return 0;
}