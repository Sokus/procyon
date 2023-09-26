#include "pe_platform_psp.h"

#include <stdbool.h>

#include <pspkernel.h>

PSP_MODULE_INFO("Procyon Engine", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static bool should_quit = false;

static int pe_exit_callback(int arg1, int arg2, void *common) {
    should_quit = true;
    return 0;
}

static int pe_exit_callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", pe_exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

bool pe_platform_init_psp(void) {
	int thid = sceKernelCreateThread("update_thread", pe_exit_callback_thread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
    return (thid >= 0);
}

bool pe_platform_shutdown_psp(void) {
	sceKernelExitGame();
}

bool pe_platform_should_quit_psp(void) {
    return should_quit;
}