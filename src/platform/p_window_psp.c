#include "platform/p_window.h"

#include "utility/pe_trace.h"

#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h> // sceDisplayWaitVblankStart
PSP_MODULE_INFO("Procyon Engine", 0, 1, 1); // TODO: parametrize module name
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);

struct peWindowStatePSP {
    bool should_quit;
} p_window_state_psp;

static int pe_exit_callback_thread(SceSize args, void *argp);

void pe_window_platform_init(int, int, const char *) {
    int thid = sceKernelCreateThread("update_thread", pe_exit_callback_thread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
}

void pe_window_platform_shutdown(void) {
    sceKernelExitGame();
}

bool pe_window_should_quit(void) {
    return p_window_state_psp.should_quit;
}

void pe_window_poll_events(void) {
}

void pe_window_swap_buffers(bool vsync) {
    PE_TRACE_FUNCTION_BEGIN();
    if (vsync) {
        sceDisplayWaitVblankStart();
    }
    sceGuSwapBuffers();
    PE_TRACE_FUNCTION_END();
}

////////////////////////////////////////////////////////////////////////////////

static int pe_exit_callback(int/*arg1*/, int/*arg2*/, void*/*common*/) {
    p_window_state_psp.should_quit = true;
    return 0;
}

static int pe_exit_callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", pe_exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}
