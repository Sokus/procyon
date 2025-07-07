/*
    Hello World example made by Aurelio Mannara for libctru
    This code was modified for the last time on: 12/12/2014 21:00 UTC+1
*/

#include <3ds.h>
#include <stdio.h>
#include <stdint.h>

#include "core/p_assert.h"
#include "core/p_time.h"

int main(int argc, char **argv)
{
    gfxInitDefault();

    //Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
    consoleInit(GFX_TOP, NULL);

    // uint64_t start_tick = p_time_now();
    uint64_t start_tick = svcGetSystemTick();

    uint64_t tick = p_time_now();

    // Main loop
    static int frame = 0;
    while (aptMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break; // break in order to return to hbmenu

        if (frame % 5 == 0) {
            consoleClear();

            // uint64_t elapsed = p_time_since(start_tick);
            // uint64_t elapsed = svcGetSystemTick() - start_tick;
            uint64_t elapsed = p_time_since(tick);
            // SYSCLOCK_ARM11
            printf("frame: %d\n", frame);
            printf("time [ticks]: %llu\n", elapsed);
            printf("time [ms]: %llu\n", elapsed/(SYSCLOCK_ARM11/1000));
            printf("time [ms new]: %f\n", p_time_ms(elapsed));
            printf("time [s]: %f\n", p_time_sec(elapsed));
        }
        frame += 1;

        // Flush and swap framebuffers
        gfxFlushBuffers();
        gfxSwapBuffers();

        //Wait for VBlank
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}