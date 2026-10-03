/* Adapted from BirchWoodGod/halo-ce-vita, 309b9de, host/vita_main.c.
 * GPL-3.0-only; see LICENSES/BirchWoodGod-GPL-3.0.txt.
 *
 * Halo's recursive UI and world code must not depend on the Vita startup
 * thread's default stack. Keep all engine/GL initialization and teardown on
 * this one thread. Only the process owner starts, joins and deletes it.
 */
#include "vita_runtime.h"
#include <psp2/kernel/threadmgr.h>

#define HALO_ENGINE_STACK_BYTES (16u * 1024u * 1024u)

static int vita_engine_thread(SceSize size, void *arguments)
{
	int status;
	if (size != sizeof(int) || !arguments)
		return 1;
	vita_log("[VITA BOOT] engine thread started id=%d stack=%u bytes",
		sceKernelGetThreadId(), HALO_ENGINE_STACK_BYTES);
	status = halo_vita_run(*(const int *)arguments);
	sceKernelExitThread(status);
	return status;
}

int vita_run_engine(int platform)
{
	SceUID thread;
	int result, status = 1;
	if (!platform) {
		vita_log("[VITA BOOT] engine blocked: native platform initialization failed");
		return 1;
	}
	thread = sceKernelCreateThread("HaloCE engine", vita_engine_thread,
		0x10000100, HALO_ENGINE_STACK_BYTES, 0, 0, NULL);
	if (thread < 0) {
		vita_log("[VITA BOOT] engine thread creation failed result=0x%08x stack=%u",
			thread, HALO_ENGINE_STACK_BYTES);
		return 1;
	}
	result = sceKernelStartThread(thread, sizeof(platform), &platform);
	if (result < 0) {
		vita_log("[VITA BOOT] engine thread start failed id=%d result=0x%08x", thread, result);
		sceKernelDeleteThread(thread);
		return 1;
	}
	result = sceKernelWaitThreadEnd(thread, &status, NULL);
	if (result < 0) {
		/* The thread may still own Halo/GL resources. The process exits without
		 * deleting that live thread or invoking teardown on a different thread. */
		vita_log("[VITA BOOT] engine thread join failed id=%d result=0x%08x", thread, result);
		return 1;
	}
	result = sceKernelDeleteThread(thread);
	if (result < 0) {
		vita_log("[VITA BOOT] engine thread delete failed id=%d result=0x%08x", thread, result);
		return 1;
	}
	return status;
}
