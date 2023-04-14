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
#include "pe_protocol.h"

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

#include <stdio.h>

int main(int argc, char **argv)
{
	SetupCallbacks();
	pspDebugScreenInit();

	pe_time_init();
	pe_net_init();

	for (int i = 0; i < 50; i += 1) {
		pe_net_update();
		pe_time_sleep(100);
	}

	fprintf(stdout, "sending the packet\n");

    peSocket socket;
    pe_socket_create(peSocket_IPv4, 0, &socket);

    peAddress server = pe_address4(192, 168, 128, 81, 54727);

    peMessageType type = peMessage_ConnectionClosed;
    peConnectionRequestMessage *msg = pe_alloc_message(pe_heap_allocator(), type);
    pePacket packet = {0};
    pe_append_message(&packet, msg, type);

    if (pe_send_packet(socket, server, &packet)) {
		fprintf(stdout, "packet sent.\n");
	}

	pe_net_shutdown();

	sceKernelExitGame();
	return 0;
}