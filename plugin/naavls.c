#include <stdio.h>
#include <string.h>
#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>

#define LOG_LOC "ux0:data/noautoavls.log"

#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		logg(buffer, strlen(buffer), LOG_LOC, 2); \
} while (0)
	
#define LOG_START(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		logg(buffer, strlen(buffer), LOG_LOC, 1); \
} while (0)

static char leafbuf[512], leafobuf[512];
static tai_hook_ref_t hook_ref;
static int hook = 0;

static int logg(void *buffer, int length, const char* logloc, int create)
{
	int fd;
	if (create == 0) {
		fd = ksceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND, 6);
	} else if (create == 1) {
		fd = ksceIoOpen(logloc, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	} else if (create == 2) {
		fd = ksceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 6);
	}
	if (fd < 0)
		return 0;

	ksceIoWrite(fd, buffer, length);
	ksceIoClose(fd);
	return 1;
}

int siofix(void *func) {
	int ret = 0;
	int res = 0;
	int uid = 0;
	ret = uid = ksceKernelCreateThread("siofix", func, 64, 0x10000, 0, 0, 0);
	if (ret < 0){ret = -1; goto cleanup;}
	if ((ret = ksceKernelStartThread(uid, 0, NULL)) < 0) {ret = -1; goto cleanup;}
	if ((ret = ksceKernelWaitThreadEnd(uid, &res, NULL)) < 0) {ret = -1; goto cleanup;}
	ret = res;
cleanup:
	if (uid > 0) ksceKernelDeleteThread(uid);
	return ret;
}

static int ReOrSet118() {
	memset(leafbuf, 0, 0x200);
	unsigned int iset = 0, ret;
	int xr = ksceIdStorageLookup(0x118, 0, &iset, 1);
	if (xr != 0) {
		LOG("no leaf, creating... ");
		ret = ksceIdStorageCreateLeaf(0x118);
		if (ret != 0) {
			LOG("0x%X\n", ret);
			return 1;
		}
		LOG("OK!\n");
		LOG("restarting idstorage... ");
		ret = ksceIdStorageRestart(1);
		if (ret != 0) {
			LOG("0x%X\n", ret);
			return 1;
		}
		LOG("OK!\n");
		leafbuf[0] = 1;
		LOG("writing data... ");
		ret = ksceIdStorageWriteLeaf(0x118, leafbuf, 1);
		if (ret != 0) {
			LOG("0x%X\n", ret);
			return 1;
		}
		LOG("OK!\n");
	} else if (xr == 0) {
		LOG("leaf exists!\n");
		if (iset == 0) {
			LOG("AutoAvls disabled, enabling... ");
			leafbuf[0] = 1;
			ret = ksceIdStorageWriteLeaf(0x118, leafbuf, 1);
			if (ret != 0) {
				LOG("0x%X\n", ret);
				return 2;
			}
			LOG("OK!\n");
			LOG("restarting idstorage... ");
			ret = ksceIdStorageRestart(1);
			if (ret != 0) {
				LOG("0x%X\n", ret);
				return 1;
			}
			ret = ksceIdStorageReadLeaf(0x118, leafobuf, 1);
			if (ret != 0)
				return 2;
			LOG("byte0: 0x%X\n", leafbuf[0]);
		} else {
			LOG("AutoAvls enabled, disabling... ");
			ret = ksceIdStorageWriteLeaf(0x118, leafbuf, 1);
			if (ret != 0) {
				LOG("0x%X\n", ret);
				return 2;
			}
			LOG("OK!\n");
			LOG("restarting idstorage... ");
			ret = ksceIdStorageRestart(1);
			if (ret != 0) {
				LOG("0x%X\n", ret);
				return 1;
			}
			ret = ksceIdStorageReadLeaf(0x118, leafobuf, 1);
			if (ret != 0)
				return 2;
			LOG("byte0: 0x%X\n", leafbuf[0]);
		}
	}
	ret = ksceIdStorageRestart(0);
	LOG("ReOrSet118: 0x%X, 0x%X\n", xr, ret);
	return 0;
}

static int proxy_ReOrSet118() {
	int state = 0, ret = 0;
	ENTER_SYSCALL(state);
	ret = siofix(ReOrSet118);
	EXIT_SYSCALL(state);
	return ret;
}

SceUID is_pm_patched(void *buf) {
	*(uint8_t *)buf = 4;
	return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	LOG_START("naavls started!\n");
	int sysroot = ksceKernelGetSysbase();
	LOG("fw: 0x%lX\n", *(uint32_t *)(*(int *)(sysroot + 0x6c) + 4));
	hook = taiHookFunctionImportForKernel(KERNEL_PID, &hook_ref, "SceIdStorage", 0xF13F32F9, 0x2AC815A2, is_pm_patched);
	proxy_ReOrSet118();
	LOG("naavls finished!\n");
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	taiHookReleaseForKernel(hook, hook_ref);
	return SCE_KERNEL_STOP_SUCCESS;
}
