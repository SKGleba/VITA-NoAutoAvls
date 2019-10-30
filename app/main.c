/*
 * Simple kplugin loader by xerpi
 */

#include <stdio.h>
#include <taihen.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include "debugScreen.h"

#define MOD_PATH "ux0:app/SKG4V2SW1/naavls.skprx"
	
#define printf(...) psvDebugScreenPrintf(__VA_ARGS__)

void wait_key_press()
{
	SceCtrlData pad;

	printf("Press CROSS to change AVLS mode.\n");

	while (1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if (pad.buttons & SCE_CTRL_CROSS)
			break;
		sceKernelDelayThread(200 * 1000);
	}
}

int main(int argc, char *argv[])
{
	int ret;
	SceUID mod_id;
	psvDebugScreenInit();
	printf("NoAutoAvls v1.0\n");
	wait_key_press();
	printf("\nChanging AVLS mode...");
	tai_module_args_t argg;
	argg.size = sizeof(argg);
	argg.pid = KERNEL_PID;
	argg.args = 0;
	argg.argp = NULL;
	argg.flags = 0;
	mod_id = taiLoadStartKernelModuleForUser(MOD_PATH, &argg);
	if (mod_id > 0) {
		printf("\nok!, rebooting in 5s\n");
		printf("you can check the log for more info\n");
		sceKernelDelayThread(5 * 1000 * 1000);
		scePowerRequestColdReset();
	} else {
		printf("\nerr, check log!\n");
		sceKernelDelayThread(5 * 1000 * 1000);
	}
	return 0;
}
