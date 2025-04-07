#include "platform/p_window.h"

#include "core/p_assert.h"
#include "utility/pe_trace.h"

#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h> // sceDisplayWaitVblankStart
PSP_MODULE_INFO("Procyon Engine", 0, 1, 1); // TODO: parametrize module name
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);

struct peWindowStatePSP {
    bool should_quit;
} p_window_state_psp = {0};

static int p_window_exit_callback_thread(SceSize args, void *argp);

void p_window_platform_init(int, int, const char *) {
    int thid = sceKernelCreateThread("update_thread", p_window_exit_callback_thread, 0x11, 0xFA0, 0, 0);
    P_ASSERT(thid >= 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
}

void p_window_platform_shutdown(void) {
    sceKernelExitGame();
}

bool p_window_should_quit(void) {
    return p_window_state_psp.should_quit;
}

void p_window_poll_events(void) {
}

void p_window_swap_buffers(bool vsync) {
    P_TRACE_FUNCTION_BEGIN();
    if (vsync) {
        sceDisplayWaitVblankStart();
    }
    sceGuSwapBuffers();
    P_TRACE_FUNCTION_END();
}

////////////////////////////////////////////////////////////////////////////////

static int p_window_exit_callback(int/*arg1*/, int/*arg2*/, void*/*common*/) {
    p_window_state_psp.should_quit = true;
    return 0;
}

static int p_window_exit_callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", p_window_exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return cbid;
}
