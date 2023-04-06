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
#include <string.h>

#include "pe_core.h"
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

char *pe_psp_apctl_state_to_string(int apctl_state) {
	switch(apctl_state) {
		case PSP_NET_APCTL_STATE_DISCONNECTED: return "PSP_NET_APCTL_STATE_DISCONNECTED";
		case PSP_NET_APCTL_STATE_SCANNING:     return "PSP_NET_APCTL_STATE_SCANNING";
		case PSP_NET_APCTL_STATE_JOINING:      return "PSP_NET_APCTL_STATE_JOINING";
		case PSP_NET_APCTL_STATE_GETTING_IP:   return "PSP_NET_APCTL_STATE_GETTING_IP";
		case PSP_NET_APCTL_STATE_GOT_IP:       return "PSP_NET_APCTL_STATE_GOT_IP";
		case PSP_NET_APCTL_STATE_EAP_AUTH:     return "PSP_NET_APCTL_STATE_EAP_AUTH";
		case PSP_NET_APCTL_STATE_KEY_EXCHANGE: return "PSP_NET_APCTL_STATE_KEY_EXCHANGE";
	}
}

/* Simple thread */
int main(int argc, char **argv)
{
	SetupCallbacks();
	pspDebugScreenInit();

	pe_net_init();

	pspDebugScreenPrintf("Attempting Network Init\n");
	sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    //sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEURI);
    //sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEHTTP);
    //sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP);

	sceNetInit(0x20000, 0x20, 0x1000, 0x20, 0x1000);
	sceNetInetInit();
	sceNetApctlInit(0x1600, 42);

    int err;
    int stateLast = -1;
    err = sceNetApctlConnect(1);
    if (err != 0) {
		pspDebugScreenPrintf("error: sceNetApctlConnect(1)\n");
		sceKernelExitGame();
        return 0;
    }

    while (1) {
        int state;
        err = sceNetApctlGetState(&state);
        if (err != 0) {
			pspDebugScreenPrintf("error: sceNetApctlGetState(&state)\n");
			sceKernelExitGame();
        return 0;
        }
        if (state > stateLast) {
            stateLast = state;
			pspDebugScreenPrintf("%s\n", pe_psp_apctl_state_to_string(state));
        }
        if (state == 4)
            break; // connected with static IP

        // wait a little before polling again
        sceKernelDelayThread(50 * 1000); // 50ms
    }

	pspDebugScreenPrintf("Connected!\n");

	union SceNetApctlInfo info;
	if (sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) >= 0) {
		pspDebugScreenPrintf("IP: %s\n", info.ip);
	} else {
		pspDebugScreenPrintf("sceNetApctlGetInfo(8, &info) error\n");
	}

	peSocket socket = pe_socket_create(peSocket_IPv4, 0);
	int data;
	peAddress server = pe_address4(192, 168, 128, 81, 54727);
	char address_string[256];
	pspDebugScreenPrintf("sending %d bytes to %s\n", sizeof(data), pe_address_to_string(server, address_string, sizeof(address_string)));
	pe_socket_send(socket, server, &data, sizeof(data));

	sceKernelDelayThread(5 * 1000 * 1000);
	sceKernelExitGame();
	return 0;
}