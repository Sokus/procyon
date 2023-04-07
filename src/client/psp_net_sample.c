#include <malloc.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <pspmodulemgr.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <psppower.h>
#include <psptypes.h>
#include <psputility.h>
#include <psputils.h>
#include <stdarg.h>
#include <psprtc.h>
#include <string.h>

#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

PSP_MODULE_INFO("Procyon", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread,
				     0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}

int main(int argc, char **argv)
{
	SetupCallbacks();
	pspDebugScreenInit();

	pe_time_init();
	pe_net_init();

	peSocket socket = pe_socket_create(peSocket_IPv4, 0);
	float dt = 1.0f;
	for (int i = 0; i < 30; i += 1) {
		pe_net_update();
		pspDebugScreenPrintf("waiting for a packet\n");
		int data;
		peAddress address;
		pe_socket_receive(socket, &address, &data, sizeof(data));
		//pe_socket_send(socket, pe_address4(192, 168, 128, 81, 54727), &data, sizeof(data));
		pe_time_sleep(dt*1000.0f);
	}

	pe_net_shutdown();

	sceKernelExitGame();
	return 0;
}